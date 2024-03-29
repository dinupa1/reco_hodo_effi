#include <iomanip>
#include <TFile.h>
#include <TTree.h>

#include <fun4all/Fun4AllReturnCodes.h>
#include <phool/getClass.h>
#include <interface_main/SQHitVector_v1.h>
#include <interface_main/SQTrackVector_v1.h>
#include <interface_main/SQDimuonVector_v1.h>
#include <interface_main/SQRun.h>
#include <interface_main/SQEvent.h>
#include <vector>

#include "AnaModule.h"
#include "HitData.h"

AnaModule::AnaModule(const std::string& name): SubsysReco(name), p_geomSvc(GeomSvc::instance())
{}

AnaModule::~AnaModule()
{}

int AnaModule::Init(PHCompositeNode* topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

int AnaModule::InitRun(PHCompositeNode* topNode)
{
  int ret = GetNodes(topNode);
  if(ret != Fun4AllReturnCodes::EVENT_OK) return ret;

  eventID = 0;
  MakeTree();
  return Fun4AllReturnCodes::EVENT_OK;
}

int AnaModule::process_event(PHCompositeNode* topNode)
{
	int nTracklets = trackletVec->size();
	for(int i = 0; i < nTracklets; ++i)
	{
		Tracklet* tracklet = trackletVec->at(i);
		nHits = tracklet->getNHits();
		chisq = tracklet->getChisq();

		//very loose cuts here
		if(nHits < 5) continue;
		if(chisq > 15.) continue;

		// data structure created by dinupa3@gmail.com
		if(event->get_trigger(SQEvent::NIM1) == 1) {trigger = 1;}
		if(event->get_trigger(SQEvent::NIM2) == 1) {trigger = 2;}
		if(event->get_trigger(SQEvent::NIM3) == 1) {trigger = 3;}
		if(event->get_trigger(SQEvent::NIM4) == 1) {trigger = 4;}
		if(event->get_trigger(SQEvent::MATRIX5) == 1) {trigger =5;}

		//std::cout << "trigger bit " << trigger << std::endl;

		std::vector<int> detIDs;
		std::vector<int> eleID_exps;
		std::vector<int> eleID_closests;
		
		//std::cout << "nHits : " << nHits << " chisq : " << chisq << std::endl;

    for(auto it = detectorIDs.begin(); it != detectorIDs.end(); ++it)
    {
      detectorID = *it;

      double z_exp = p_geomSvc->getPlanePosition(detectorID);
      x_exp = tracklet->getExpPositionX(z_exp);
      y_exp = tracklet->getExpPositionY(z_exp);
      if(!p_geomSvc->isInPlane(detectorID, x_exp, y_exp)) continue;

      elementID_exp = p_geomSvc->getExpElementID(detectorID, tracklet->getExpPositionW(detectorID));
      if(elementID_exp < 1 || elementID_exp > p_geomSvc->getPlaneNElements(detectorID)) continue;

      SQHit* hit = findHit(detectorID, elementID_exp);
      elementID_closest = hit == nullptr ? -1 : hit->get_element_id();

			detIDs.push_back(detectorID);
			eleID_exps.push_back(elementID_exp);
			eleID_closests.push_back(elementID_closest);
		}

		int num_hits = detIDs.size();

		D0 = 0;
		D2 = 0;
		D3 = 0;
		P1 = 0;
		diff1 = 0;
		diff2 = 0;
		diff3 = 0;
		diff4 = 0;

		for(int ii = 0; ii < num_hits; ii++){
			int detID0 = detIDs.at(ii);
			int expID0 = eleID_exps.at(ii);
			int closestID0 = eleID_closests.at(ii);

			// D0
			if(0 < detID0 && detID0 < 7 && closestID0 > 0){
				D0 += 1;
				diff1 += abs(expID0 - closestID0);
			}

			// D2
			if(12 < detID0 && detID0 < 19 && closestID0 > 0){
				D2 += 1;
				diff2 += abs(expID0 - closestID0);
			}

			// D3
			if((18 < detID0 && detID0 < 25 && closestID0 > 0) || (24 < detID0 && detID0 < 31 && closestID0 > 0)){
				D3 += 1;
				diff3 += abs(expID0 - closestID0);
			}

			// PT
			if((46 < detID0 && detID0 < 51 && closestID0 > 0) || (50 < detID0 && detID0 < 55 && closestID0 > 0)){
				P1 += 1;
				diff4 += abs(expID0 - closestID0);
			}
		}

		for(int ii = 0; ii < num_hits; ii++){

			int detID0 = detIDs.at(ii);
			int expID0 = eleID_exps.at(ii);
			int closestID0 = eleID_closests.at(ii);

			if(30 < detID0 && detID0 < 47){
				detID = detID0;
				eleID_exp = expID0;
				eleID_closest = closestID0;
			}

			saveTree->Fill();
		}

		//saveTree->Fill();
		//hit_vec.clear();
  }
  ++eventID;
  return Fun4AllReturnCodes::EVENT_OK;
}

int AnaModule::End(PHCompositeNode* topNode)
{
  saveFile->cd();
  saveTree->Write();
  saveFile->Close();

  return Fun4AllReturnCodes::EVENT_OK;
}

int AnaModule::GetNodes(PHCompositeNode* topNode)
{
  hitVector   = findNode::getClass<SQHitVector>(topNode, "SQHitVector");
  trackletVec = findNode::getClass<TrackletVector>(topNode, "TrackletVector");
	event       = findNode::getClass<SQEvent    >(topNode, "SQEvent");

  if(!hitVector || !trackletVec || !event)
  {
    return Fun4AllReturnCodes::ABORTRUN;
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

void AnaModule::MakeTree()
{
  saveFile = new TFile(saveName, "RECREATE");

  saveTree = new TTree("save", "Efficiency tree Created by AnaModule");
  saveTree->Branch("eventID", &eventID, "eventID/I");
	saveTree->Branch("trigger", &trigger, "trigger/I");
  saveTree->Branch("detID", &detID, "detID/I");
  saveTree->Branch("eleID_exp", &eleID_exp, "eleID_exp/I");
  saveTree->Branch("eleID_closest", &eleID_closest, "eleID_closest/I");
	saveTree->Branch("D0", &D0, "D0/I");
	saveTree->Branch("D2", &D2, "D2/I");
	saveTree->Branch("D3", &D3, "D3/I");
	saveTree->Branch("P1", &P1, "P1/I");
	saveTree->Branch("diff1", &diff1, "diff1/I");
	saveTree->Branch("diff2", &diff2, "diff2/I");
	saveTree->Branch("diff3", &diff3, "diff3/I");
	saveTree->Branch("diff4", &diff4, "diff4/I");
	//saveTree->Branch("x_exp", &x_exp, "x_exp/D");
  //saveTree->Branch("y_exp", &y_exp, "y_exp/D");
  saveTree->Branch("nHits", &nHits, "nHits/I");
  saveTree->Branch("chisq", &chisq, "chisq/D");
	//saveTree->Branch("hit_vec", &hit_vec);
}

void AnaModule::registerDetector(TString name)
{
  detectorIDs.insert(p_geomSvc->getDetectorID(name.Data()));
}

SQHit* AnaModule::findHit(int detID, int eleID)
{
  int delta = 999;
  SQHit* hit = nullptr;
  for(SQHitVector::Iter it = hitVector->begin(); it != hitVector->end(); ++it)
  {
    if((*it)->get_detector_id() != detID) continue;
    int delta_temp = abs(eleID - (*it)->get_element_id());
    if(delta > delta_temp)
    {
      delta = delta_temp;
      hit = (*it);
    }
  }

  return hit;
}
