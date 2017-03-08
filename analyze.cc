#ifndef __ANALYZE__
#define __ANALYZE__

#include <iostream>
#include <dirent.h>
#include "ConsoleColor.h"

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
#include "TDirectory.h"
#include "TMultiGraph.h"

int gCanvasCounter = 0;

//class BiasSweepData : public TObject
//{
//public:
//uint16_t fFeId;
//uint16_t fCbcId;
//std::string fBias;
//long int fTimestamp;
//char fUnit[2];
//uint16_t fInitialXValue;
//float fInitialYValue;
//std::vector<uint16_t> fXValues;
//std::vector<float> fYValues;

//BiasSweepData() : fFeId (0), fCbcId (0), fBias (""), fTimestamp (0)
//{
//}
//~BiasSweepData() {}
//cTree->Branch ("Bias", &fData->fBias);
//cTree->Branch ("Fe", &fData->fFeId, "Fe/s" );
//cTree->Branch ("Cbc", &fData->fCbcId, "Cbc/s" );
//cTree->Branch ("Time", &fData->fTimestamp, "Time/l" );
//cTree->Branch ("Unit", &fData->fUnit, "Unit/C" );
//cTree->Branch ("InitialBiasValue", &fData->fInitialXValue, "InitialDAC/s");
//cTree->Branch ("InitialDMMValue", &fData->fInitialYValue, "InitialDMM/F");
//cTree->Branch ("BiasValues", &fData->fXValues);
//cTree->Branch ("DMMValues", &fData->fYValues);

//};

std::vector<std::string> list_folders (std::string pDirectory, std::string pFilename)
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
            cFilename += "/";
            cFilename += pFilename;
            cFilelist.push_back (cFilename);
            //std::cout << cFilename << std::endl;
        }

        entry = readdir (dir);
    }

    closedir (dir);
    return cFilelist;
}

void merge_files (std::string pDatadir, std::string pOutfile)
{
    std::vector<std::string> cFileList = list_folders (pDatadir, "Temperature_log.txt");
    std::ofstream of (pOutfile);

    if (of.is_open() )
    {
        for (auto& cFilename : cFileList)
        {
            std::cout << YELLOW << cFilename << RESET << std::endl;
            std::ifstream cFile (cFilename);
            std::string line;

            if (cFile.is_open() )
            {
                while ( std::getline (cFile, line) )
                {
                    if (line.empty() )                 // be careful: an empty line might be read
                        continue;
                    else if (line.find ("#") == std::string::npos && line.find ("Temperature") == std::string::npos)
                    {
                        of << line << std::endl;
                        std::cout << line << std::endl;
                    }
                }

                cFile.close();
            }
        }

        of.close();
    }
}

float get_temperature (std::string pTFile, long int pTimestamp)
{
    float cTemperature = 0;
    std::ifstream cFile (pTFile);

    if (cFile.is_open() )
    {
        long int cTimestamp = 0;
        float cReadTemperature = 0;
        std::string cUnit;

        while (!cFile.eof() )
        {
            cFile >> cTimestamp >> cReadTemperature >> cUnit;

            //consider it good when within 50 seconds
            if (fabs (pTimestamp - cTimestamp) < 30)
            {
                cTemperature = cReadTemperature;
                //std::cout << "Found " << cTimestamp << " close to " << pTimestamp << std::endl;
                break;
            }

            //else std::cout << BOLDRED << "ERROR, could not find a temperature reading within 30s of the timestamp!" << RESET << std::endl;
        }
    }
    else std::cout  << "ERROR opening temperature file!" << std::endl;

    return cTemperature;
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
                std::cout << GREEN << cName << " " << year << "." << month << "." << day << " " << hour << ":" << minute << ":" << seconds << " which is " << tsstart.GetSec() << " in UTC" << RESET << std::endl;

                if (!cTimefile.eof() )
                {
                    cTimefile >> cName >> year >> month >> day >> hour >> minute >> seconds;

                    if (cName == "stop")
                    {
                        //-1 because I need UTC
                        TTimeStamp tsstop (year, month, day, hour - 1, minute, seconds);
                        tmppair.second = static_cast<long int> (tsstop.GetSec() );
                        std::cout << RED << cName << " " << year << "." << month << "." << day << " " << hour << ":" << minute << ":" << seconds << " which is " << tsstop.GetSec() << " in UTC" << RESET << std::endl;

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

float get_dose (timepair pTimepair, long int pTimestamp)
{
    float cDoserate = pTimepair.doserate / 3600;
    float cDose = 0;

    //first, calculate the accumulated dose until timestamp
    for (auto cTimepair : pTimepair.timepair)
    {
        //the timestamp is greater than the stop of the current period, accumulate
        if (pTimestamp > cTimepair.second)
            cDose += cDoserate * (cTimepair.second - cTimepair.first);

        //timestamp is between start and stop: add until timestamp and then stop
        if (pTimestamp > cTimepair.first && pTimestamp < cTimepair.second)
        {
            cDose += cDoserate * (pTimestamp - cTimepair.first);
            break;
        }

        //timestamp is smaller than the next start, return present value and stop
        if (pTimestamp < cTimepair.first) break;
    }

    TTimeStamp ts (static_cast<time_t> (pTimestamp) );
    //std::cout << RED << "Dose at: " << pTimestamp << " which is: " << ts.AsString ("lc") << " : " << cDose << " kGy" << RESET << std::endl;
    return cDose;
}


TGraph* draw_time (timepair pTimepair, TGraph* pGraph)
{
    //timepair pTimepair = get_times (pTimefile);
    float cDoserate = pTimepair.doserate / 3600.; // 20kGy/h / 3600s
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
    long int cMaxDoseTime = 0;

    for (auto cTimepair : pTimepair.timepair)
    {
        cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimepair.first, cDose);

        //both start and end were earlier than the last point in the graph
        if (cTimepair.second != 0 && cTimestamp > cTimepair.second)
        {
            cDose += cDoserate * (cTimepair.second - cTimepair.first);
            cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimepair.second, cDose);
            cMaxDoseTime = cTimepair.second;
        }
        //  end is later than the last point in the graph
        //  set the last point to the dose at timestamp
        else if (cTimepair.second != 0 && cTimestamp < cTimepair.second)
        {
            cDose += cDoserate * (cTimestamp - cTimepair.first);
            cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimestamp, cDose);
            cMaxDoseTime = cTimestamp;
        }
        //there is no end and thus the last point in the graph is the latest
        else if (cTimepair.second == 0 && cTimestamp > cTimepair.first )
        {
            cDose += (cTimestamp - cTimepair.first) * cDoserate;
            cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimestamp, cDose);
            cMaxDoseTime = cTimestamp;
        }
    }

    //now calculate the maximal dose at the latest point in time (latest measurement in the graph or last stop) - this value needs to fit on the graph
    float cMaxDose = cDose;
    //get the minimum and maximum of the Y axis - and scale that range by the max dose
    float cScaleFactor = (pGraph->GetYaxis()->GetXmax() - pGraph->GetYaxis()->GetXmin() ) / cMaxDose;
    TTimeStamp ts (static_cast<time_t> (cMaxDoseTime) );
    std::cout << BLUE << "max dose: " << cMaxDose << " at time: " << ts.AsString ("lc") << " min Y " << cMinY << " max Y: " << cMaxY << " scale: " << cScaleFactor << RESET << std::endl;
    //done filling the dose graph, I still need to scale it
    cN = cDoseGraph->GetN();
    cY = cDoseGraph->GetY();

    for (int i = 0; i < cN; i++)
        cY[i] = cY[i] * cScaleFactor + cMinY;


    //plot
    cDoseGraph->GetXaxis()->SetTimeDisplay (1);
    cDoseGraph->SetLineWidth (2);
    cDoseGraph->SetLineColor (2);

    TString cCanvasName = Form ("%s_%d", pGraph->GetTitle(), gCanvasCounter);
    gCanvasCounter++;
    TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName);
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

void draw_dose (TGraph* pGraph)
{
    TString cCanvasName = Form ("%s_%d", pGraph->GetTitle(), gCanvasCounter);
    gCanvasCounter++;
    TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName);
    cCanvas->cd();
    pGraph->Draw ("AP");
}

void plot_sweep (std::string pDatadir, timepair pTimepair, std::string pBias)
{
    std::set<std::string> cSweeps{"VCth", "CAL_Vcasc", "VPLUS1", "VPLUS2", "VBGbias", "Ipa", "Ipre1", "Ipre2", "CAL_I", "Ibias", "Ipsf", "Ipaos", "Icomp", "Ihyst"};

    if (cSweeps.find (pBias) == std::end (cSweeps) ) std::cout << BOLDRED << "ERROR: " << pBias << " is not a sweep!" << RESET << std::endl;
    else
    {

        TMultiGraph* cGraph = new TMultiGraph();
        std::vector<std::string> cFileList = list_folders (pDatadir, "Cbc3RadiationCycle.root");

        int cColor = 1;

        for (auto& cFilename : cFileList)
        {
            if (cFilename.find ("FULL") != std::string::npos)
            {
                TFile* cFile = TFile::Open (cFilename.c_str() );
                TDirectory* cDir = dynamic_cast< TDirectory* > ( gROOT->FindObject ("FE0CBC0") );


                if (cFile != nullptr)
                {
                    for (auto cKey : *cDir->GetListOfKeys() )
                    {
                        std::string cGraphName = static_cast<std::string> (cKey->GetName() );

                        if (cGraphName.find (pBias) != std::string::npos)
                        {
                            TGraph* cTmpGraph;// = static_cast<TGraph*> (cKey->ReadObj() );
                            cDir->GetObject (cGraphName.c_str(), cTmpGraph);
                            cTmpGraph->SetLineColor (cColor);
                            long int cTimestamp = atoi (cGraphName.substr (cGraphName.find ("TS") + 2).c_str() );
                            cTmpGraph->SetTitle (Form ("%3.0f %s", get_dose (pTimepair, cTimestamp), "kGy" ) );
                            cGraph->Add (cTmpGraph);
                            //std::cout << cGraphName << " " << cTimestamp << std::endl;
                            cColor++;
                        }
                    }
                }
                else std::cout << BOLDRED << "ERROR, could not open File: " << cFilename << RESET << std::endl;
            }
        }

        TString cCanvasName = Form ("%s_%d", pBias.c_str(), gCanvasCounter);
        gCanvasCounter++;
        TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName);
        cCanvas->cd();
        cGraph->Draw ("AL");
        cCanvas->BuildLegend();
    }
}

void plot_bias (std::string pDatadir, timepair pTimepair, std::string pBias, std::string pParameter)
{
    TGraph* cGraph = new TGraph();
    gROOT->ProcessLine ("#include <vector>");
    std::vector<std::string> cFileList = list_folders (pDatadir, "Cbc3RadiationCycle.root");

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
                    int cBiasSetting = 0;
                    float cValue = 0;
                    std::vector<uint16_t>* cBiasVector = nullptr;
                    std::vector<float>* cDMMVector = nullptr;

                    auto cBiasBranch = cTree->GetBranch ("Bias");
                    auto cTimeBranch  = cTree->GetBranch ("Time");
                    auto cInitialXBranch  = cTree->GetBranch ("InitialBiasValue");
                    auto cInitialYBranch  = cTree->GetBranch ("InitialDMMValue");
                    auto cBiasVectorBranch = cTree->GetBranch ("BiasValues");
                    auto cDMMVectorBranch = cTree->GetBranch ("DMMValues");

                    cBiasBranch->SetAddress (&cBias);
                    cTimeBranch->SetAddress (&cTimestamp);
                    cInitialXBranch->SetAddress (&cBiasSetting);
                    cInitialYBranch->SetAddress (&cValue);
                    cBiasVectorBranch->SetAddress (&cBiasVector);
                    cDMMVectorBranch->SetAddress (&cDMMVector);

                    auto cNentries = cTree->GetEntries();

                    for (unsigned int i = 0; i < cNentries; i++)
                    {
                        cTree->GetEntry (i);

                        //std::cout << *cBias << " " << cTimestamp << " " << cValue << std::endl;

                        if (*cBias == pBias)
                        {
                            //std::cout << *cBias << " " << cTimestamp << " " << cValue << std::endl;
                            if (pParameter == "time")
                                cGraph->SetPoint (cGraph->GetN(), cTimestamp, cValue);
                            else if (pParameter == "dose")
                                cGraph->SetPoint (cGraph->GetN(), get_dose (pTimepair, cTimestamp), cValue);
                            else if (pParameter == "temperature")
                                cGraph->SetPoint (cGraph->GetN(), get_temperature ("TLog_chip0.txt", cTimestamp), cValue);
                        }
                    }
                }
                else std::cout << "error loading treee" << std::endl;
            }
        }
    }

    cGraph->GetYaxis()->SetTitle (pBias.c_str() );
    cGraph->GetYaxis()->SetTitleOffset (1.25);
    cGraph->SetMarkerStyle (8);
    cGraph->SetTitle (pBias.c_str() );

    if (pParameter == "time")
    {
        cGraph->GetXaxis()->SetTimeDisplay (1);
        cGraph->GetXaxis()->SetTitle ("Time");
        draw_time (pTimepair, cGraph);
    }
    else if (pParameter == "dose")
    {
        cGraph->GetXaxis()->SetTitle ("Dose [kGy]");
        draw_dose (cGraph);
    }
    else if (pParameter == "temperature")
    {
        cGraph->GetXaxis()->SetTitle ("Temperature [C]");
        draw_dose (cGraph);
    }
}

void analyze()
{
    //timepair cTimepair = get_times ("timefile_chip1");
    //plot_bias ("Chip1_358kGy", cTimepair, "VBG_LDO");
    //plot_bias ("Chip1_358kGy", cTimepair, "MinimalPower");

    timepair cTimepair = get_times ("timefile_chip0");
    plot_sweep ("Data/Chip0_55kGy", cTimepair, "VCth");
    plot_bias ("Data/Chip0_55kGy", cTimepair, "VBG_LDO", "dose");
    plot_bias ("Data/Chip0_55kGy", cTimepair, "VBG_LDO", "time");
    plot_bias ("Data/Chip0_55kGy", cTimepair, "VBG_LDO", "temperature");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "MinimalPower", "dose");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "VCth", "time");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "VBGbias");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "Vpafb");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "VPLUS1");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "VPLUS2");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "Ipre1");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "Ipre2");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "Ipaos");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "Ipsf");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "Icomp");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "Ihyst");
}
// key(BiasSweep, reg name, amux code, bit mask, bit shift
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("none"),   std::make_tuple ("", 0x00, 0x00, 0) ); fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Ipa"),    std::make_tuple ("Ipa", 0x01, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Ipre2"),  std::make_tuple ("Ipre2", 0x02, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("CAL_I"),  std::make_tuple ("CALIbias", 0x03, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Ibias"),  std::make_tuple ("Ibias", 0x04, 0x00, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("VCth"),    std::make_tuple ("", 0x05, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("VBGbias"), std::make_tuple ("BandgapFuse", 0x06, 0x3F, 0) );//read this on the VDDA line?
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("VBG_LDO"), std::make_tuple ("", 0x07, 0x00, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Vpafb"),  std::make_tuple ("", 0x08, 0x00, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Nc50"),   std::make_tuple ("", 0x09, 0x00, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Ipre1"),  std::make_tuple ("Ipre1", 0x0A, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Ipsf"),   std::make_tuple ("Ipsf", 0x0B, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Ipaos"),  std::make_tuple ("Ipaos", 0x0C, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Icomp"),  std::make_tuple ("Icomp", 0x0D, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("Ihyst"),  std::make_tuple ("FeCtrl&TrgLat2", 0x0E, 0x3C, 2) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("CAL_Vcasc"), std::make_tuple ("CALVcasc", 0x0F, 0xFF, 0) );
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("VPLUS2"), std::make_tuple ("Vplus1&2", 0x10, 0xF0, 4) ) ;
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("VPLUS1"), std::make_tuple ("Vplus1&2", 0x11, 0x0F, 0) ) ;
//fAmuxSettings.emplace (std::piecewise_construct, std::make_tuple ("VDDA"), std::make_tuple ("", 0x00, 0x00, 0) ) ;
#endif
