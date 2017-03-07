#ifndef __ANALYZE__
#define __ANALYZE__

#include <iostream>
#include <dirent.h>

#include "TROOT.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TF1.h"
#include "TFile.h"
#include "TString.h"
#include "TDirectory.h"
#include "TTree.h"
#include "TH1D.h"
#include "TMath.h"
#include "TGaxis.h"
#include "TTimeStamp.h"


class BiasSweepData : public TObject
{
  public:
    uint16_t fFeId;
    uint16_t fCbcId;
    std::string fBias;
    long int fTimestamp;
    char fUnit[2];
    uint16_t fInitialXValue;
    float fInitialYValue;
    std::vector<uint16_t> fXValues;
    std::vector<float> fYValues;

    BiasSweepData() : fFeId (0), fCbcId (0), fBias (""), fTimestamp (0)
    {
    }
    ~BiasSweepData() {}
    //cTree->Branch ("Bias", &fData->fBias);
    //cTree->Branch ("Fe", &fData->fFeId, "Fe/s" );
    //cTree->Branch ("Cbc", &fData->fCbcId, "Cbc/s" );
    //cTree->Branch ("Time", &fData->fTimestamp, "Time/l" );
    //cTree->Branch ("Unit", &fData->fUnit, "Unit/C" );
    //cTree->Branch ("InitialBiasValue", &fData->fInitialXValue, "InitialDAC/s");
    //cTree->Branch ("InitialDMMValue", &fData->fInitialYValue, "InitialDMM/F");
    //cTree->Branch ("BiasValues", &fData->fXValues);
    //cTree->Branch ("DMMValues", &fData->fYValues);

};

std::vector<std::string> list_folders (std::string pDirectory)
{
    std::vector<std::string> cFilelist;
    DIR* dir = opendir (pDirectory.c_str() );

    struct dirent* entry = readdir (dir);

    while (entry != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            std::string cFilename = pDirectory + "/";
            cFilename += static_cast<std::string> (entry->d_name);
            cFilename += "/Cbc3RadiationCycle.root";
            cFilelist.push_back (cFilename);
            std::cout << cFilename << std::endl;
        }

        entry = readdir (dir);
    }

    closedir (dir);
    return cFilelist;
}

struct timepair
{
    std::vector<std::pair<long int, long int>> timepair; //pair of start and stop timestamps, if running last stop is 0
    float doserate; //kGy/h
};

timepair get_times (std::string pTimefile)
{
    std::ifstream cTimefile;
    cTimefile.open (pTimefile.c_str() );
    timepair cPair;

    if (cTimefile.is_open() )
    {
        std::string cName;
        float cDoserate;
        int year, month, day, hour, minute, seconds;

        cTimefile >> cName >> cDoserate;
        std::cout << cName << ": " << cDoserate << " kGy/h" << std::endl;

        if (cName != "doserate") std::cout << "ERROR in file format" << std::endl;
        else cPair.doserate = cDoserate;

        std::pair<long int, long int> tmppair;

        while (!cTimefile.eof() )
        {
            cTimefile >> cName >> year >> month >> day >> hour >> minute >> seconds;

            if (cName == "start")
            {
                //-1 because I need UTC
                TTimeStamp tsstart (year, month, day, hour - 1, minute, seconds);
                tmppair.first = static_cast<long int> (tsstart.GetSec() );
                std::cout << cName << " " << year << "." << month << "." << day << " " << hour << ":" << minute << ":" << seconds << " which is " << tsstart.GetSec() << " in UTC" << std::endl;

                if (!cTimefile.eof() )
                {
                    cTimefile >> cName >> year >> month >> day >> hour >> minute >> seconds;

                    if (cName == "stop")
                    {
                        //-1 because I need UTC
                        TTimeStamp tsstop (year, month, day, hour - 1, minute, seconds);
                        tmppair.second = static_cast<long int> (tsstop.GetSec() );
                        std::cout << cName << " " << year << "." << month << "." << day << " " << hour << ":" << minute << ":" << seconds << " which is " << tsstop.GetSec() << " in UTC" << std::endl;

                    }
                    else std::cout << "ERROR in the file format" << std::endl;
                }
                else if (cTimefile.eof() )
                    tmppair.second = 0;

                cPair.timepair.push_back (tmppair);
            }
        }
    }
    else std::cout << "ERROR opening timefile " << pTimefile << std::endl;

    //for (auto cTime : cPair.timepair) std::cout << cTime.first << " " << cTime.second << std::endl;

    return cPair;
}

TGraph* draw_dose (std::string pTimefile, TGraph* pGraph)
{
    timepair cTimes = get_times (pTimefile);
    float cDoserate = cTimes.doserate / 3600.; // 20kGy/h / 3600s
    //first, analyze the passed graph
    int cN = pGraph->GetN();
    double* cY = pGraph->GetY();
    double* cX = pGraph->GetX();
    //int cLocMax = TMath::LocMax (cN, cY);
    float cMaxY = pGraph->GetYaxis()->GetXmax();
    //float cMaxY = cY[cLocMax];
    //cLocMax = TMath::LocMin (cN, cY);
    float cMinY = pGraph->GetYaxis()->GetXmin();
    //float cMinY = cY[cLocMax];
    int cLocMax = TMath::LocMax (cN, cX);
    long int cTimestamp = cX[cLocMax];
    //long int cTimestamp = pGraph->GetXaxis()->GetXmax();


    //then, fill the dose graph
    TGraph* cDoseGraph = new TGraph();
    float cDose = 0;

    for (auto cTimepair : cTimes.timepair)
    {
        cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimepair.first, cDose);

        //both start and end were earlier than the last point in the graph
        if (cTimepair.second != 0 && cTimestamp > cTimepair.second)
        {
            cDose += cDoserate * (cTimepair.second - cTimepair.first);
            cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimepair.second, cDose);
        }
        //  end is later than the last point in the graph
        //  set the last point to the dose at timestamp
        else if (cTimepair.second != 0 && cTimestamp < cTimepair.second)
        {
            cDose += cDoserate * (cTimestamp - cTimepair.first);
            cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimestamp, cDose);
        }
        //there is no end and thus the last point in the graph is the latest
        else if (cTimepair.second == 0 && cTimestamp > cTimepair.first )
        {
            cDose += (cTimestamp - cTimepair.first) * cDoserate;
            cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimestamp, cDose);
        }
    }

    //now calculate the maximal dose at the latest point in time (latest measurement in the graph or last stop) - this value needs to fit on the graph
    float cMaxDose = cDose;
    //get the minimum and maximum of the Y axis - and scale that range by the max dose
    float cScaleFactor = (pGraph->GetYaxis()->GetXmax() - pGraph->GetYaxis()->GetXmin() ) / cMaxDose;
    std::cout << "max dose: " << cMaxDose << " min Y " << cMinY << " max Y: " << cMaxY << " scale: " << cScaleFactor << std::endl;
    //done filling the dose graph, I still need to scale it
    cN = cDoseGraph->GetN();
    cY = cDoseGraph->GetY();

    for (int i = 0; i < cN; i++)
        cY[i] = cY[i] * cScaleFactor + cMinY;


    //plot
    cDoseGraph->GetXaxis()->SetTimeDisplay (1);
    cDoseGraph->SetLineWidth (2);
    cDoseGraph->SetLineColor (2);

    TCanvas* cCanvas = new TCanvas (pGraph->GetTitle(), pGraph->GetTitle() );
    cCanvas->cd();
    pGraph->Draw ("AP");

    TGaxis* cAxis = new TGaxis (pGraph->GetXaxis()->GetXmax(), pGraph->GetYaxis()->GetXmin(), pGraph->GetXaxis()->GetXmax(), pGraph->GetYaxis()->GetXmax(), 0, cMaxDose, 510, "+L");
    cAxis->SetTitle ("Dose [kGy]");
    cAxis->Draw ("same");
    cAxis->SetLineColor (2);
    cAxis->SetLabelColor (2);
    cAxis->SetTitleColor (2);

    cDoseGraph->Draw ("PL same");
    cCanvas->Modified();
    cCanvas->Update();

    return cDoseGraph;
}





void plot_bias (std::string pDatadir, std::string pTimefile, std::string pBias)
{
    //get_times ("pTimefile");
    TGraph* cGraph = new TGraph();
    gROOT->ProcessLine ("#include <vector>");
    std::vector<std::string> cFileList = list_folders (pDatadir);

    for (auto& cFilename : cFileList)
    {
        if (cFilename.find ("FULL") != std::string::npos)
        {
            TFile* cFile = TFile::Open (cFilename.c_str() );

            if (cFile != nullptr)
            {
                //cFile->cd ("FE0CBC0");
                TTree* cTree = nullptr;
                cFile->GetObject ("FE0CBC0/BiasSweep_Fe0_Cbc0", cTree);

                if (cTree != nullptr)
                {
                    std::string* cBias = nullptr;
                    long int  cTimestamp = 0;
                    float cValue = 0;

                    auto cBiasBranch = cTree->GetBranch ("Bias");
                    auto cTimeBranch  = cTree->GetBranch ("Time");
                    auto cInitialYBranch  = cTree->GetBranch ("InitialDMMValue");

                    cBiasBranch->SetAddress (&cBias);
                    cTimeBranch->SetAddress (&cTimestamp);
                    cInitialYBranch->SetAddress (&cValue);

                    auto cNentries = cTree->GetEntries();

                    for (unsigned int i = 0; i < cNentries; i++)
                    {
                        cTree->GetEntry (i);

                        std::cout << *cBias << " " << cTimestamp << " " << cValue << std::endl;

                        if (*cBias == pBias)
                        {
                            //std::cout << *cBias << " " << cTimestamp << " " << cValue << std::endl;
                            cGraph->SetPoint (cGraph->GetN(), cTimestamp, cValue);
                        }
                    }
                }
                else std::cout << "error loading treee" << std::endl;
            }
        }
    }

    cGraph->GetXaxis()->SetTimeDisplay (1);
    cGraph->GetYaxis()->SetTitle (pBias.c_str() );
    cGraph->GetYaxis()->SetTitleOffset (1.25);
    cGraph->GetXaxis()->SetTitle ("Time");
    cGraph->SetMarkerStyle (8);
    cGraph->SetTitle (pBias.c_str() );

    //TCanvas* c1 = new TCanvas ("c1", "c1");
    draw_dose (pTimefile, cGraph);
}

void analyze()
{
    plot_bias ("Data", "timefile_chip1", "VBG_LDO");
    plot_bias ("Data", "timefile_chip1", "MinimalPower");
}
#endif
