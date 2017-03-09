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

void merge_files (std::string pDatadir, std::string pOutfile, bool pTemperature = true)
{
    std::vector<std::string> cFileList;

    if (pTemperature) cFileList = list_folders (pDatadir, "Temperature_log.txt");
    else cFileList = list_folders (pDatadir, "Current_log.txt");

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
                    else if (line.find ("#") == std::string::npos && line.find ("Temperature") == std::string::npos && line.find ("Logfile") == std::string::npos)
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


TGraph* draw_time (timepair pTimepair, TGraph* pGraph, int pChipId)
{
    //timepair pTimepair = get_times (pTimefile);
    float cDoserate = pTimepair.doserate / 3600.; // 20kGy/h / 3600s
    //first, analyze the passed graph
    int cN = pGraph->GetN();
    double* cY = pGraph->GetY();
    double* cX = pGraph->GetX();
    float cMaxY = pGraph->GetYaxis()->GetXmax();
    float cMinY = pGraph->GetYaxis()->GetXmin();
    //float cMinY = cY[cLocMax];
    int cLocMax = TMath::LocMax (cN, cX);
    long int cTimestamp = cX[cLocMax];


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

    //scaling and filling temperature graph
    for (int i = 0; i < cN; i++)
        cY[i] = cY[i] * cScaleFactor + cMinY;

    //now go through the graph, get the xvalues=timestamps
    //calculate the temperature for the timestamps and fill a dedicated temperature graph that I draw ina special pad
    cN = pGraph->GetN();
    cX = pGraph->GetX();
    Double_t y[cN];

    //scaling and filling temperature graph
    for (int i = 0; i < cN; i++)
        y[i] = get_temperature (Form ("TLog_chip%1d.txt", pChipId), cX[i]);

    TGraph* cTemperatureGraph = new TGraph (cN, cX, y);
    cTemperatureGraph->GetXaxis()->SetTimeDisplay (1);
    cTemperatureGraph->SetMarkerColor (4);
    //cTemperatureGraph->SetMarkerSize (4);
    cTemperatureGraph->SetMarkerStyle (2);
    cTemperatureGraph->GetYaxis()->SetTitle ("Temperature [C]");
    cTemperatureGraph->GetYaxis()->SetLabelSize (0.1);
    cTemperatureGraph->GetXaxis()->SetLabelSize (0.12);
    cTemperatureGraph->GetYaxis()->SetTitleSize (0.12);
    cTemperatureGraph->GetYaxis()->SetTitleOffset (0.3);


    //plot
    cDoseGraph->GetXaxis()->SetTimeDisplay (1);

    cDoseGraph->SetLineWidth (2);
    cDoseGraph->SetLineColor (2);

    TString cCanvasName = Form ("%s_%d", pGraph->GetTitle(), gCanvasCounter);
    gCanvasCounter++;
    TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName);
    TPad* lowerPad = new TPad ("lowerPad", "lowerPad", .005, .005, .995, .2475);
    lowerPad->Draw();
    TPad* upperPad = new TPad ("upperPad", "upperPad", .005, .2525, .995, .995);
    upperPad->Draw();

    upperPad->cd();
    pGraph->Draw ("AP");

    TGaxis* cAxis = new TGaxis (pGraph->GetXaxis()->GetXmax(), pGraph->GetYaxis()->GetXmin(), pGraph->GetXaxis()->GetXmax(), pGraph->GetYaxis()->GetXmax(), 0, cMaxDose, 510, "+L");
    cAxis->SetTitle ("Dose [kGy]");
    cAxis->Draw ("same");
    cAxis->SetLineColor (2);
    cAxis->SetLabelColor (2);
    cAxis->SetTitleColor (2);

    cDoseGraph->Draw ("PL same");

    lowerPad->cd();
    cTemperatureGraph->Draw ("AP");
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

void plot_pedenoise (std::string pDatadir, timepair pTimepair, std::string pHistName, std::string pParameter)
{
    int cChipId = atoi (pDatadir.substr (pDatadir.find ("_") - 1, 1).c_str() );
    std::cout << pDatadir << " Chip id extracted " << cChipId << std::endl;
    TGraph* cGraph = new TGraph();
    std::vector<std::string> cFileList = list_folders (pDatadir, "Cbc3RadiationCycle.root");

    if (pHistName == "Pedestal") pHistName = "Fe0CBC0_Pedestal";
    else if (pHistName == "Noise") pHistName = "Fe0CBC0_Noise";

    for (auto& cFilename : cFileList)
    {
        //if (cFilename.find ("FULL") != std::string::npos)
        //{
        TFile* cFile = TFile::Open (cFilename.c_str() );
        TDirectory* cDir = dynamic_cast< TDirectory* > ( gROOT->FindObject ("FE0CBC0") );


        if (cFile != nullptr)
        {
            for (auto cKey : *cDir->GetListOfKeys() )
            {
                std::string cGraphName = static_cast<std::string> (cKey->GetName() );

                //if (cGraphName.find (pHistName ) != std::string::npos)
                if (cGraphName == pHistName)
                {
                    TH1F* cTmpHist;// = static_cast<TGraph*> (cKey->ReadObj() );
                    cDir->GetObject (cGraphName.c_str(), cTmpHist);
                    //here need to parse the directory name
                    std::string cTimestring = cFilename.substr (cFilename.find (":") - 11, 14);
                    //std::cout << cTimestring << std::endl;
                    //int year = 2017;
                    int day = atoi (cTimestring.substr (0, 2).c_str() ); //07-03-17_09:50
                    int month = atoi (cTimestring.substr (3, 2).c_str() ); //07-03-17_09:50
                    int year = atoi (cTimestring.substr (6, 2).c_str() ); //07-03-17_09:50
                    int hour = atoi (cTimestring.substr (9, 2).c_str() ); //07-03-17_09:50
                    int minute = atoi (cTimestring.substr (12, 2).c_str() ); //07-03-17_09:50
                    TTimeStamp ts (year, month, day, hour - 1, minute, 0);
                    //std::cout << year << " " << month << " " << day << " " << hour << " " << minute << " ts " << ts.GetSec() << std::endl;
                    long int cTimestamp = static_cast<long int> (ts.GetSec() );
                    std::cout << cGraphName << " " << cKey->GetName() << std::endl;

                    if (pParameter == "time")
                        cGraph->SetPoint (cGraph->GetN(), cTimestamp, cTmpHist->GetMean() );
                    else if (pParameter == "dose")
                        cGraph->SetPoint (cGraph->GetN(), get_dose (pTimepair, cTimestamp), cTmpHist->GetMean() );
                    else if (pParameter == "temperature")
                        cGraph->SetPoint (cGraph->GetN(), get_temperature (Form ("TLog_chip%1d.txt", cChipId), cTimestamp), cTmpHist->GetMean() );
                }
            }
        }
        else std::cout << BOLDRED << "ERROR, could not open File: " << cFilename << RESET << std::endl;

        //}
    }

    cGraph->GetYaxis()->SetTitle (pHistName.c_str() );
    cGraph->GetYaxis()->SetTitleOffset (1.25);
    cGraph->SetMarkerStyle (8);
    cGraph->SetTitle (pHistName.c_str() );

    if (pParameter == "time")
    {
        cGraph->GetXaxis()->SetTimeDisplay (1);
        cGraph->GetXaxis()->SetTitle ("Time");
        draw_time (pTimepair, cGraph, cChipId);
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

                        if (cGraphName.find (pBias + "_") != std::string::npos)
                        {
                            TGraph* cTmpGraph;// = static_cast<TGraph*> (cKey->ReadObj() );
                            cDir->GetObject (cGraphName.c_str(), cTmpGraph);
                            cTmpGraph->SetLineColor (cColor);
                            long int cTimestamp = atoi (cGraphName.substr (cGraphName.find ("TS") + 2).c_str() );
                            cTmpGraph->SetTitle (Form ("%3.0f %s", get_dose (pTimepair, cTimestamp), "kGy" ) );
                            cGraph->Add (cTmpGraph);
                            std::cout << cGraphName << " " << cKey->GetName() << std::endl;
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
    int cChipId = atoi (pDatadir.substr (pDatadir.find ("_") - 1, 1).c_str() );
    std::cout << pDatadir << " Chip id extracted " << cChipId << std::endl;
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
                                cGraph->SetPoint (cGraph->GetN(), get_temperature (Form ("TLog_chip%1d.txt", cChipId), cTimestamp), cValue);
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
        draw_time (pTimepair, cGraph, cChipId);
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
    //plot_sweep ("Data/Chip0_55kGy", cTimepair, "Ipre1");
    //plot_sweep ("Data/Chip0_55kGy", cTimepair, "Ipa");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "VBG_LDO", "dose");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "VBG_LDO", "time");
    //plot_bias ("Data/Chip0_55kGy", cTimepair, "VBG_LDO", "temperature");
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
    plot_pedenoise ("Data/Chip0_55kGy", cTimepair, "Pedestal", "time");
    plot_pedenoise ("Data/Chip0_55kGy", cTimepair, "Noise", "time");
}
#endif
