#ifndef __ANALYZE__
#define __ANALYZE__

#include <iostream>
//#include <dirent.h>
#include <glob.h>
#include "ConsoleColor.h"
#include <fstream>

#include "TROOT.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TFile.h"
#include "TString.h"
#include "TDirectory.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2.h"
#include "TMath.h"
#include "TGaxis.h"
#include "TTimeStamp.h"
#include "TDirectory.h"
#include "TMultiGraph.h"
#include "TLegend.h"

int gCanvasCounter = 0;
int gCounter = 0;
bool gDaylightSavingTime = true;


const char* create_filename (std::string pDatadir, std::string pName)
{
    int cChipId = atoi (pDatadir.substr (pDatadir.find ("_") - 1, 1).c_str() );
    int cDose = atoi (pDatadir.substr (pDatadir.find ("_") + 1, pDatadir.find ("kGy") - pDatadir.find ("_") ).c_str() );
    std::string tmp = Form ("Results/Chip%1d_%03dkGy/%s_Chip%1d_%03dkGy.png", cChipId, cDose, pName.c_str(), cChipId, cDose);
    return tmp.c_str();
}

void format_timeaxis (TGraph* pGraph)
{
    pGraph->GetXaxis()->SetTimeDisplay (1);
    pGraph->GetXaxis()->SetTimeFormat ("%d-%Hh");
    //pGraph->GetXaxis()->SetTimeOffset (0, "gmt");
}

std::vector<std::string> list_folders (std::string pDirectory, std::string pFilename)
{
    glob_t glob_result;
    pDirectory += "/*";
    glob (pDirectory.c_str(), GLOB_TILDE | GLOB_MARK, NULL, &glob_result);
    std::vector<std::string> dirs;

    for (unsigned int i = 0; i < glob_result.gl_pathc; ++i)
    {
        if (std::string (glob_result.gl_pathv[i]).back() == '/')
            dirs.push_back (std::string (glob_result.gl_pathv[i])  + pFilename );

        //std::cout << std::string (glob_result.gl_pathv[i]) +  pFilename << std::endl;
    }

    globfree (&glob_result);
    return dirs;
}
//this does not work because of unterminated #define in Availability.h (apple!)
//std::vector<std::string> cFilelist;
//DIR* dir = opendir (pDirectory.c_str() );

//struct dirent* entry = readdir (dir);

//while (entry != NULL)
//{
//if (entry->d_type == DT_DIR)
//{
//std::string cFilename = pDirectory + "/";

//if (static_cast<std::string> (entry->d_name).find (".") == std::string::npos)
//{
//cFilename += static_cast<std::string> (entry->d_name);

//cFilename += "/";
//cFilename += pFilename;
//cFilelist.push_back (cFilename);
////std::cout << cFilename << std::endl;
//}
//}

//entry = readdir (dir);
//}

//closedir (dir);
//std::sort (cFilelist.begin(), cFilelist.end() );
//return cFilelist;
//}

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
    //pTimestamp is in local
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

            if (gDaylightSavingTime) cTimestamp += (1 * 3600);

            //else cTimestamp += 3600;

            //consider it good when within 50 seconds
            if (fabs (pTimestamp - cTimestamp) < 30 && cReadTemperature > -50)
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

TGraph* get_temperatureGraph (std::string pTFile)
{
    std::ifstream cFile (pTFile);
    TGraph* cGraph = new TGraph();

    if (cFile.is_open() )
    {
        long int cTimestamp = 0;
        float cReadTemperature = 0;
        std::string cUnit;

        while (!cFile.eof() )
        {
            cFile >> cTimestamp >> cReadTemperature >> cUnit;

            if (gDaylightSavingTime) cTimestamp += (1 * 3600);

            //else cTimestamp += 3600;

            if (cReadTemperature > -50) cGraph->SetPoint (cGraph->GetN(), cTimestamp, cReadTemperature);
        }

    }
    else std::cout  << "ERROR opening temperature file!" << std::endl;

    return cGraph;
}

void format_temperatureGraph (TGraph* cGraph)
{
    cGraph->SetTitle ("");
    cGraph->GetYaxis()->SetNdivisions (010, kTRUE);
    format_timeaxis (cGraph);
    cGraph->SetMarkerColor (4);
    cGraph->SetMarkerSize (.4);
    cGraph->SetMarkerStyle (2);
    cGraph->GetYaxis()->SetTitle ("Temperature [C]");
    cGraph->GetYaxis()->SetLabelSize (0.1);
    cGraph->GetXaxis()->SetLabelSize (0.12);
    cGraph->GetYaxis()->SetTitleSize (0.12);
    cGraph->GetYaxis()->SetTitleOffset (0.3);
}

struct timepair
{
    std::vector<std::pair<long int, long int>> timepair; //pair of start and stop timestamps, if running last stop is 0
    float doserate; //kGy/h
    float temperature; // C
};

timepair get_times (std::string pTimefile)
{
    std::ifstream cTimefile;
    cTimefile.open (pTimefile.c_str() );
    timepair cPair;

    int cOffset = (gDaylightSavingTime) ? 3600 : 0;

    if (cTimefile.is_open() )
    {
        std::string cName;
        float cDoserate;
        float cTemperature;
        int year, month, day, hour, minute, seconds;

        cTimefile >> cName >> cDoserate;
        std::cout << YELLOW << cName << ": " << cDoserate << " kGy/h" << RESET << std::endl;

        if (cName != "doserate") std::cout << "ERROR in file format" << std::endl;
        else cPair.doserate = cDoserate;

        cTimefile >> cName >> cTemperature;
        std::cout << YELLOW << cName << ": " << cTemperature << " C" << RESET << std::endl;

        if (cName != "temperature") std::cout << "ERROR in file format" << std::endl;
        else cPair.temperature = cTemperature;


        std::pair<long int, long int> tmppair;

        while (!cTimefile.eof() )
        {
            cTimefile >> cName >> year >> month >> day >> hour >> minute >> seconds;

            if (cName == "start")
            {

                //TODO
                TTimeStamp tsstart (year, month, day, hour, minute, seconds, 0, false, cOffset);
                tmppair.first = static_cast<long int> (tsstart.GetSec() );
                std::cout << GREEN << cName << " " << year << "." << month << "." << day << " " << hour << ":" << minute << ":" << seconds << " which is " << tsstart.GetSec() << " in UTC" << RESET << std::endl;

                if (!cTimefile.eof() )
                {
                    cTimefile >> cName >> year >> month >> day >> hour >> minute >> seconds;

                    if (cName == "stop")
                    {
                        //TODO
                        TTimeStamp tsstop (year, month, day, hour, minute, seconds, 0, false, cOffset);
                        tmppair.second = static_cast<long int> (tsstop.GetSec() );
                        std::cout << RED << cName << " " << year << "." << month << "." << day << " " << hour << ":" << minute << ":" << seconds << " which is " << tsstop.GetSec() << " in UTC" << RESET << std::endl;

                    }
                    else
                    {
                        tmppair.second = 0;
                        std::cout << "ERROR in the file format  or no stop specified" << std::endl;
                    }
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
    //timestamp in local, timepair to be seen
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

TGraph* get_dosegraph (timepair pTimepair, long int pTimestamp_first, long int pTimestamp_last)
{
    //begin and end in local
    //then, fill the dose graph
    TGraph* cDoseGraph = new TGraph();
    float cDoserate = pTimepair.doserate / 3600.; // 20kGy/h / 3600s
    float cDose = 0;
    long int cMaxDoseTime = 0;

    cDoseGraph->SetPoint (cDoseGraph->GetN(), pTimestamp_first, 0);

    for (auto cTimepair : pTimepair.timepair)
    {
        cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimepair.first, cDose);

        //both start and end were earlier than the last point in the graph
        if (cTimepair.second != 0 && pTimestamp_last > cTimepair.second)
        {
            cDose += cDoserate * (cTimepair.second - cTimepair.first);
            cDoseGraph->SetPoint (cDoseGraph->GetN(), cTimepair.second, cDose);
            cMaxDoseTime = cTimepair.second;
        }
        //  end is later than the last point in the graph
        //  set the last point to the dose at timestamp
        else if (cTimepair.second != 0 && pTimestamp_last < cTimepair.second)
        {
            cDose += cDoserate * (pTimestamp_last - cTimepair.first);
            cDoseGraph->SetPoint (cDoseGraph->GetN(), pTimestamp_last, cDose);
            cMaxDoseTime = pTimestamp_last;
        }
        //there is no end and thus the last point in the graph is the latest
        else if (cTimepair.second == 0 && pTimestamp_last > cTimepair.first )
        {
            cDose += (pTimestamp_last - cTimepair.first) * cDoserate;
            cDoseGraph->SetPoint (cDoseGraph->GetN(), pTimestamp_last, cDose);
            cMaxDoseTime = pTimestamp_last;
        }
    }

    cDoseGraph->SetPoint (cDoseGraph->GetN(), pTimestamp_last, cDose);

    return cDoseGraph;
}


TGraph* draw_time (std::string pDatadir, timepair pTimepair, TGraph* pGraph, int pChipId)
{
    //because Sarah wants it, I'm going to put the original value in the title and scale all the points to the original value
    int cPointCounter = 0;
    double cStartValue;
    double x;
    bool finished = false;

    while (!finished)
    {
        pGraph->GetPoint (cPointCounter, x, cStartValue);
        float cTemperature = get_temperature (Form ("TLog_chip%1d.txt", pChipId), x);

        //std::cout << "Nominal: " << pTimepair.temperature << " Found: " << cTemperature << " Dose: " << get_dose (pTimepair, x) << " Point: " << cPointCounter << " of: " << pGraph->GetN() << std::endl;

        if ( (fabs (pTimepair.temperature - cTemperature) < 2) && (get_dose (pTimepair, x) == 0) )
        {
            std::cout << GREEN << "Found first point " << cPointCounter << " at nominal temperature: " << cTemperature << " expected temperature " << pTimepair.temperature << " timestamp " << x << RESET << std::endl;
            finished = true;
        }
        //else if ( (fabs (pTimepair.temperature - cTemperature) < 3) && (get_dose (pTimepair, x) != 0) )
        //std::cout << YELLOW << "Found first point " << cPointCounter << " at nominal temperature: " << cTemperature << " expected temperature " << pTimepair.temperature << " timestamp " << x << " but at dose " << get_dose (pTimepair, x) <<  RESET << std::endl;

        else if (cPointCounter == pGraph->GetN() )
        {
            std::cout << "Error, someting might be wrong, can't find a corresponding temperature!" << std::endl;
            break;
        }

        cPointCounter++;
    }

    std::string cGraphTitle = static_cast<std::string> (pGraph->GetTitle() );

    //if (cGraphTitle.find ("V") != std::string::npos)
    //cGraphTitle += Form (" StartValue = %3.3f V", cStartValue);
    //else if (cGraphTitle.find ("I") != std::string::npos)
    //cGraphTitle += Form (" StartValue = %3.3f A", cStartValue);
    //else
    //cGraphTitle += Form (" StartValue = %3.3f", cStartValue);

    //pGraph->SetTitle (cGraphTitle.c_str() );

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

    float cMaxRel = 100 * ( (cMaxY - cStartValue) / cStartValue);
    float cMinRel = 100 * ( (cStartValue - cMinY) / cStartValue);


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
    std::cout << BLUE << "Creating Dose Graph with the following parameters: max dose: " << cMaxDose << " at time: " << ts.AsString ("lc") << " UTC min Y " << cMinY << " max Y: " << cMaxY << " scale: " << cScaleFactor << RESET << std::endl;
    //done filling the dose graph, I still need to scale it
    cN = cDoseGraph->GetN();
    cY = cDoseGraph->GetY();

    //scaling and filling temperature graph
    for (int i = 0; i < cN; i++)
        cY[i] = cY[i] * cScaleFactor + cMinY;

    //now go through the graph, get the xvalues=timestamps
    //calculate the temperature for the timestamps and fill a dedicated temperature graph that I draw ina special pad
    TGraph* cTemperatureGraph;

    //im not plotting the lv data, therefore find the T measurement for the data points
    std::cout << BOLDYELLOW << "Appending dose and temperature graphs for: " << pGraph->GetTitle() << RESET << std::endl;

    if (static_cast<std::string> (pGraph->GetTitle() ).find ("Ch") == std::string::npos)
    {
        cN = pGraph->GetN();
        cX = pGraph->GetX();
        Double_t y[cN];

        //scaling and filling temperature graph

        for (int i = 0; i < cN; i++)
            y[i] = get_temperature (Form ("TLog_chip%1d.txt", pChipId), cX[i]);

        cTemperatureGraph = new TGraph (cN, cX, y);
        format_temperatureGraph (cTemperatureGraph);
    }
    // I'm plotting the LV data, so get the whole T history
    else
    {
        cTemperatureGraph = get_temperatureGraph (Form ("TLog_chip%1d.txt", pChipId) );
        format_temperatureGraph (cTemperatureGraph);
    }

    //plot
    format_timeaxis (cDoseGraph);
    cDoseGraph->SetLineWidth (2);
    cDoseGraph->SetLineColor (2);

    TString cCanvasName = Form ("%s_%d", pGraph->GetTitle(), gCanvasCounter);
    gCanvasCounter++;
    TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName);
    TPad* lowerPad = new TPad ("lowerPad", "lowerPad", .005, .005, .995, .2515);
    lowerPad->Draw();
    TPad* upperPad = new TPad ("upperPad", "upperPad", .005, .2525, .995, .995);
    upperPad->Draw();

    format_timeaxis (pGraph);
    upperPad->cd();
    //get rid of the default x Axis
    pGraph->GetYaxis()->SetLabelSize (0);
    pGraph->GetYaxis()->SetLabelOffset (999);
    pGraph->GetYaxis()->SetTitleOffset (999);
    pGraph->GetYaxis()->SetTicks ("+");

    pGraph->Draw ("AP");
    TGaxis* cGraphAxis = new TGaxis (pGraph->GetXaxis()->GetXmin(), pGraph->GetYaxis()->GetXmin(), pGraph->GetXaxis()->GetXmin(), pGraph->GetYaxis()->GetXmax(), cMinY, cMaxY, 510, "=R+");
    cGraphAxis->SetTitle (pGraph->GetYaxis()->GetTitle() );
    cGraphAxis->Draw ("same");
    cGraphAxis->SetTitleOffset (1.1);
    cGraphAxis->SetLineColor (1);
    cGraphAxis->SetLabelColor (1);
    cGraphAxis->SetTitleColor (1);

    if (cMaxDose > 0)
    {
        TGaxis* cAxis = new TGaxis (pGraph->GetXaxis()->GetXmax(), pGraph->GetYaxis()->GetXmin(), pGraph->GetXaxis()->GetXmax(), pGraph->GetYaxis()->GetXmax(), 0, cMaxDose, 510, "+L");
        cAxis->SetTitle ("Dose [kGy]");
        cAxis->Draw ("same");
        cAxis->SetLineColor (2);
        cAxis->SetLabelColor (2);
        cAxis->SetTitleColor (2);

        cDoseGraph->Draw ("PL same");
    }

    if (finished)
    {
        TGaxis* cRelativeAxis = new TGaxis (pGraph->GetXaxis()->GetXmin(), pGraph->GetYaxis()->GetXmin(), pGraph->GetXaxis()->GetXmin(), pGraph->GetYaxis()->GetXmax(), -cMinRel, cMaxRel, 510, "=L-BS");
        cRelativeAxis->SetTickSize (0.02);
        cRelativeAxis->SetTitle ("Relative Value [%]");
        //cRelativeAxis->SetTitleOffset();
        cRelativeAxis->CenterTitle();
        cRelativeAxis->Draw ("same");
        cRelativeAxis->SetLabelOffset (-0.01);
        cRelativeAxis->SetLineColor (4);
        cRelativeAxis->SetLabelColor (4);
        cRelativeAxis->SetTitleColor (4);
    }

    lowerPad->cd();
    cTemperatureGraph->Draw ("AP");
    cCanvas->Modified();
    cCanvas->Update();

    //std::string cFilename = cGraphTitle.substr (0, cGraphTitle.find ("StartValue") );
    cCanvas->SaveAs (create_filename (pDatadir, cGraphTitle ) );

    return cDoseGraph;
}

void draw_dose (std::string pDatadir, TGraph* pGraph)
{
    TString cCanvasName = Form ("%s_%d", pGraph->GetTitle(), gCanvasCounter);
    gCanvasCounter++;
    TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName);
    cCanvas->cd();
    pGraph->Draw ("AP");
    cCanvas->SaveAs (create_filename (pDatadir, static_cast<std::string> (pGraph->GetTitle() ) ) );
}

void plot_lv (std::string pDatadir, timepair pTimepair, int pChannel, char pUnit)
{
    std::cout << BOLDBLUE << "Plotting LV monitoring data in data directory: " << pDatadir  << " for Channel " << pChannel << " " << pUnit << RESET << std::endl;
    int cChipId = atoi (pDatadir.substr (pDatadir.find ("_") - 1, 1).c_str() );
    std::cout << pDatadir << " Chip id extracted " << cChipId << std::endl;
    std::string cFilename = Form ("LVlog_chip%1d.txt", cChipId);

    TGraph* cGraph = new TGraph();
    std::string cBranchName = Form ("Ch%1d%c", pChannel, pUnit);

    std::ifstream cFile (cFilename);

    if (cFile.is_open() )
    {
        long int cTimestamp = 0;
        float cVoltage1, cVoltage2, cVoltage3, cVoltage4;
        float cCurrent1, cCurrent2, cCurrent3, cCurrent4;
        std::vector<float> cVoltage (4, 0);
        std::vector<float> cCurrent (4, 0);

        while (!cFile.eof() )
        {
            cFile >> cTimestamp >> cVoltage.at (0) >> cCurrent.at (0) >> cVoltage.at (1) >> cCurrent.at (1) >> cVoltage.at (2) >> cCurrent.at (2) >> cVoltage.at (3) >> cCurrent.at (3);

            //add to the timestamp since I want local
            if (gDaylightSavingTime) cTimestamp += 1 * 3600;

            //else cTimestamp += 3600;

            if (pUnit == 'V' && cVoltage.at (pChannel - 1) < 10)
                cGraph->SetPoint (cGraph->GetN(), cTimestamp, cVoltage.at (pChannel - 1) );

            if (pUnit == 'I' && cCurrent.at (pChannel - 1) < 0.8)
                cGraph->SetPoint (cGraph->GetN(), cTimestamp, cCurrent.at (pChannel - 1) );
        }

        cFile.close();
    }
    else
        std::cout << BOLDRED << "ERROR: could not open file " << cFilename << RESET << std::endl;

    cGraph->SetTitle (cBranchName.c_str() );
    cGraph->SetMarkerColor (1);
    cGraph->SetMarkerStyle (2);
    cGraph->SetMarkerSize (0.2);
    cGraph->GetXaxis()->SetTitle ("Time");
    cGraph->GetYaxis()->SetTitle (cBranchName.c_str() );
    //this works but is a real mess
    draw_time (pDatadir, pTimepair, cGraph, cChipId);
}

long int extract_timesamp (std::string pFilename)
{
    int cOffset = (gDaylightSavingTime) ? 3600 : 0;
    //in the filename the date/time string is in local time!
    //so don't do anything with it
    std::string cTimestring = pFilename.substr (pFilename.find (":") - 11, 14);
    int day = atoi (cTimestring.substr (0, 2).c_str() ); //07-03-17_09:50
    int month = atoi (cTimestring.substr (3, 2).c_str() ); //07-03-17_09:50
    int year = atoi (cTimestring.substr (6, 2).c_str() ); //07-03-17_09:50
    int hour = atoi (cTimestring.substr (9, 2).c_str() ); //07-03-17_09:50
    int minute = atoi (cTimestring.substr (12, 2).c_str() ); //07-03-17_09:50

    TTimeStamp ts (year, month, day, hour, minute, 0, 0, false, cOffset);
    long int cTimestamp = static_cast<long int> (ts.GetSec() );
    return cTimestamp;
}

double MyErf ( double* x, double* par )
{
    double x0 = par[0];
    double width = par[1];
    double fitval ( 0 );

    // if ( x[0] < x0 ) fitval = 0.5 * TMath::Erfc( ( x0 - x[0] ) / width );
    // else fitval = 0.5 + 0.5 * TMath::Erf( ( x[0] - x0 ) / width );
    if ( x[0] < x0 ) fitval = 0.5 * erfc ( ( x0 - x[0] ) / (sqrt (2.) * width ) );
    else fitval = 0.5 + 0.5 * erf ( ( x[0] - x0 ) / (sqrt (2.) * width ) );

    return fitval;
}

TF1* fit_scurve (TH1F* pScurve)
{
    gStyle->SetOptFit (0111);
    gStyle->SetOptStat (0000000000);
    //get the midpoint and width estimates for parameter start values
    double cStart = pScurve->GetBinCenter (pScurve->FindFirstBinAbove (0) );
    double cStop = pScurve->GetBinCenter (pScurve->FindFirstBinAbove (0.99) );
    double cMid = (cStop + cStart) * .5;
    double cWidth = (cStop - cStart) * .5;
    //std::cout << YELLOW << "Estimates: Start: " << cStart << " Stop: " << cStop << " Mid: " << cMid << " Width: " << cWidth << RESET <<  std::endl;

    //fitting function using error function
    TObject* cObj = nullptr;
    cObj = gROOT->FindObject ("SCurveFit");

    if (cObj != nullptr)
        delete cObj;

    TF1* cFit = new TF1 ("SCurveFit", MyErf, cStart - 10, cStop + 10, 2);

    cFit->SetParName (0, "Pedestal");
    cFit->SetParName (1, "Noise");
    cFit->SetParameter (0, cMid);
    cFit->SetParameter (1, cWidth);

    pScurve->Fit (cFit, "RNQ+");


    //std::cout << BLUE << "Reusults: Midpoint: " << cFit->GetParameter (0) << " Width: " << cFit->GetParameter (1) << " Chi^2/NDF: " << cFit->GetChisquare() / cFit->GetNDF() << RESET << std::endl;

    return cFit;
}

void draw_debug_scurves (TCanvas* pCanvas, TH1F* pHist, TF1* pFit)
{
    pCanvas->cd();
    std::string cDrawOption = (gCounter == 0) ? "PE X0" : "PE X0 same";
    //std::string cDrawOption = "PE X0 same";
    pHist->DrawCopy (cDrawOption.c_str() );
    pFit->DrawCopy ("same");
    gCounter++;
}

void plot_scurves (std::string pDatadir, timepair pTimepair, int pTPAmplitude  = 0 )
{
    //temporary for testing
    //TCanvas* cDebugCanvas = new TCanvas ("debug", "debug");

    int cCounter = 0;
    //int cHistCounter = 0;

    std::cout << BOLDBLUE << "Plotting SCurves with TP Amplitude " << pTPAmplitude << " in data directory: " << pDatadir  << RESET << std::endl;
    int cChipId = atoi (pDatadir.substr (pDatadir.find ("_") - 1, 1).c_str() );
    std::cout << pDatadir << " Chip id extracted " << cChipId << std::endl;
    std::vector<std::string> cFileList = list_folders (pDatadir, "Cbc3RadiationCycle.root");
    int cNFiles = cFileList.size();

    // get the individual time stamps of the measurements and sort them
    std::vector<long int> cTSvector;

    //times in local time from filenames
    for (auto& cFilename : cFileList)
        cTSvector.push_back (extract_timesamp (cFilename) );

    std::sort (cTSvector.begin(), cTSvector.end() );
    long int cBegin = *cTSvector.begin();
    long int cEnd = cTSvector.back();

    TH2F* cPedestal = new TH2F ("Pedestals", Form ("SCurve Midpoint_TP%d", pTPAmplitude ), cNFiles, cBegin, cEnd, 254, -.5, 253.5);
    TH2F* cNoise = new TH2F ("Noise", Form ("SCurve Width_TP%d", pTPAmplitude), cNFiles, cBegin, cEnd, 254, -.5, 253.5);

    std::string pHistName = Form ("SCurves%d", pTPAmplitude);

    int cScurveCounter = 0;
    double cMeanPedestal = 0;

    for (auto& cFilename : cFileList)
    {
        //if (cCounter == 25) break;

        TFile* cFile = TFile::Open (cFilename.c_str() );
        TDirectory* cDir = dynamic_cast< TDirectory* > ( gROOT->FindObject (pHistName.c_str() ) );

        if (cFile != nullptr && cDir != nullptr)
        {
            for (auto cKey : *cDir->GetListOfKeys() )
            {
                if (cKey == nullptr) std::cout << RED << "Error: key has a problem !" << RESET << std::endl;

                std::string cGraphName = static_cast<std::string> (cKey->GetName() );

                //local time, so no mods
                long int cTimestamp = extract_timesamp (cFilename);

                //find only SCurves, not derivatives
                if (cGraphName.find ("Scurve_") != std::string::npos)
                {
                    int cChannel = atoi (cGraphName.substr (cGraphName.find ("Channel") + 7, 3 ).c_str() );
                    TH1F* cTmpHist;// = static_cast<TGraph*> (cKey->ReadObj() );
                    cDir->GetObject (cGraphName.c_str(), cTmpHist);

                    if (cTmpHist == nullptr)
                        std::cout << RED << "Error, histogram object " << cGraphName << " for timestamp " << cTimestamp << " does not exist!" << RESET << std::endl;

                    else
                    {
                        for (int iBin = 0; iBin < cTmpHist->GetNbinsX(); iBin++)
                        {
                            float cOccupancy = cTmpHist->GetBinContent (iBin);
                            float cBinError = sqrt ( (cOccupancy * (1 - cOccupancy) ) / 200.  );
                            cTmpHist->SetBinError (iBin, cBinError );
                        }

                        TF1* cFit = fit_scurve (cTmpHist);

                        if (cFit->GetParameter (0) == 0 || cFit->GetParameter (1) == 0) std::cout << RED << cFilename << " " << cGraphName << " " << cTimestamp << RESET << std::endl;

                        float cPedestalVal = cFit->GetParameter (0);
                        float cNoiseVal = cFit->GetParameter (1);

                        if (cPedestalVal > 0 && cPedestalVal < 1024)
                        {
                            cPedestal->SetBinContent (cPedestal->FindBin (cTimestamp, cChannel), cFit->GetParameter (0) );
                            cMeanPedestal += cFit->GetParameter (0);
                            cScurveCounter++;
                        }

                        if (cNoiseVal > 0 && cNoiseVal < 50)
                            cNoise->SetBinContent (cPedestal->FindBin (cTimestamp, cChannel), cFit->GetParameter (1) );

                        //this is the debug part!
                        //if ( (cCounter > 4 && cCounter < 13) && (cPedestalVal > 490 && cPedestalVal < 580) && (cNoiseVal < 1.5 || cNoiseVal > 3.5) )
                        //{
                        //std::cout << "Found SCurve with PEdestal over 475! " << gCounter << std::endl;
                        //draw_debug_scurves (cDebugCanvas, cTmpHist, cFit);
                        //}
                    }
                }
            }

            cFile->Close();
        }
        else std::cout << BOLDRED << "ERROR, could not open File: " << cFilename << RESET << std::endl;

        cCounter++;
        std::cout << "Done with File " << cCounter << " out of " << cNFiles << std::endl;
    }

    std::cout << cMeanPedestal << "  " << cScurveCounter << std::endl;
    cMeanPedestal /= cScurveCounter;

    TGraph* cTempGraph = get_temperatureGraph (Form ("TLog_chip%1d.txt", cChipId) );
    format_temperatureGraph (cTempGraph);

    //then, fill the dose graph
    TGraph* cDoseGraph = get_dosegraph (pTimepair, cBegin, cEnd);

    format_timeaxis (cDoseGraph);
    cDoseGraph->SetTitle ("");
    cDoseGraph->GetYaxis()->SetNdivisions (010, kTRUE);
    cDoseGraph->SetLineWidth (2);
    cDoseGraph->SetLineColor (2);
    cDoseGraph->GetYaxis()->SetTitle ("Dose [kGy]");
    cDoseGraph->GetYaxis()->SetLabelSize (0.1);
    cDoseGraph->GetXaxis()->SetLabelSize (0.12);
    cDoseGraph->GetYaxis()->SetTitleSize (0.12);
    cDoseGraph->GetYaxis()->SetTitleOffset (0.3);


    TString cCanvasName = Form ("%s_%d", "SCurve Parameters", gCanvasCounter);
    gCanvasCounter++;
    TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName, 800, 1200);
    TPad* lowestpad = new TPad ("lowestpad", "lowestpad", .005, .005, .995, .1500);
    lowestpad->Draw();
    TPad* lowerpad = new TPad ("lowerpad", "lowerpad", .005, .1505, .995, .3000);
    lowerpad->Draw();
    TPad* middlePad = new TPad ("middlePad", "middlePad", .005, .3005, .995, .6400);
    middlePad->Draw();
    TPad* upperPad = new TPad ("upperPad", "upperPad", .005, .6405, .995, .9905);
    upperPad->Draw();

    lowestpad->cd();
    cTempGraph->Draw ("AP");
    cTempGraph->GetXaxis()->SetRangeUser (cBegin, cEnd);
    cTempGraph->Draw ("AP");
    lowerpad->cd();
    cDoseGraph->Draw ("APL");
    cDoseGraph->GetXaxis()->SetRangeUser (cBegin, cEnd);
    cDoseGraph->Draw ("APL");
    cCanvas->Modified();
    cCanvas->Update();
    cPedestal->GetXaxis()->SetTimeDisplay (1);
    cNoise->GetXaxis()->SetTimeDisplay (1);
    cPedestal->GetZaxis()->SetRangeUser (cMeanPedestal - 20, cMeanPedestal + 20);
    cNoise->GetZaxis()->SetRangeUser (0, 5);
    cPedestal->GetXaxis()->SetTitle ("Time");
    cNoise->GetXaxis()->SetTitle ("Time");
    cPedestal->GetYaxis()->SetTitle ("Channel");
    cNoise->GetYaxis()->SetTitle ("Channel");
    middlePad->cd();
    cPedestal->Draw ("colz");
    upperPad->cd();
    cNoise->Draw ("colz");
    cCanvas->SaveAs (create_filename (pDatadir, Form ("SCurves_TP%d", pTPAmplitude ) ) );
}

void plot_pedenoise (std::string pDatadir, timepair pTimepair, std::string pHistName, std::string pParameter)
{
    std::cout << BOLDBLUE << "Plotting " << pHistName << " in data directory: " << pDatadir  << " against " << pParameter << RESET << std::endl;
    int cChipId = atoi (pDatadir.substr (pDatadir.find ("_") - 1, 1).c_str() );
    std::cout << pDatadir << " Chip id extracted " << cChipId << std::endl;
    TGraphErrors* cGraph = new TGraphErrors();
    std::vector<std::string> cFileList = list_folders (pDatadir, "Cbc3RadiationCycle.root");

    if (pHistName == "Pedestal") pHistName = "Fe0CBC0_Pedestal";
    else if (pHistName == "Noise") pHistName = "Fe0CBC0_Noise";

    for (auto& cFilename : cFileList)
    {
        //if (cFilename.find ("FULL") != std::string::npos)
        //{
        TFile* cFile = TFile::Open (cFilename.c_str() );
        TDirectory* cDir = dynamic_cast< TDirectory* > ( gROOT->FindObject ("FE0CBC0") );


        if (cFile != nullptr && cDir != nullptr)
        {
            for (auto cKey : *cDir->GetListOfKeys() )
            {
                std::string cGraphName = static_cast<std::string> (cKey->GetName() );

                //if (cGraphName.find (pHistName ) != std::string::npos)
                if (cGraphName == pHistName)
                {
                    TH1F* cTmpHist;// = static_cast<TGraph*> (cKey->ReadObj() );
                    cDir->GetObject (cGraphName.c_str(), cTmpHist);
                    //local time, all good
                    long int cTimestamp = extract_timesamp (cFilename);

                    int cPoint = cGraph->GetN();

                    if (pParameter == "time")
                    {
                        cGraph->SetPoint (cPoint, cTimestamp, cTmpHist->GetMean() );
                        //cGraph->SetPointError (cPoint, 0, cTmpHist->GetRMS() );
                    }
                    else if (pParameter == "dose")
                    {
                        cGraph->SetPoint (cPoint, get_dose (pTimepair, cTimestamp), cTmpHist->GetMean() );
                        //cGraph->SetPointError (cPoint, 0, cTmpHist->GetRMS() );
                    }
                    else if (pParameter == "temperature")
                    {
                        cGraph->SetPoint (cPoint, get_temperature (Form ("TLog_chip%1d.txt", cChipId), cTimestamp), cTmpHist->GetMean() );
                        //cGraph->SetPointError (cPoint, 0, cTmpHist->GetRMS() );
                    }
                }
            }

            cFile->Close();
        }
        else std::cout << BOLDRED << "ERROR, could not open File: " << cFilename << RESET << std::endl;

        //}
    }

    cGraph->GetYaxis()->SetTitle (pHistName.c_str() );
    cGraph->GetYaxis()->SetTitleOffset (1.25);
    cGraph->SetMarkerStyle (8);
    cGraph->SetMarkerSize (.4);
    cGraph->SetTitle (pHistName.c_str() );

    if (pParameter == "time")
    {
        cGraph->GetXaxis()->SetTitle ("Time");
        draw_time (pDatadir, pTimepair, cGraph, cChipId);
    }
    else if (pParameter == "dose")
    {
        cGraph->GetXaxis()->SetTitle ("Dose [kGy]");
        draw_dose (pDatadir, cGraph);
    }
    else if (pParameter == "temperature")
    {
        cGraph->GetXaxis()->SetTitle ("Temperature [C]");
        draw_dose (pDatadir, cGraph);
    }
}

void plot_sweep (std::string pDatadir, timepair pTimepair, std::string pBias)
{
    int cChipId = atoi (pDatadir.substr (pDatadir.find ("_") - 1, 1).c_str() );
    int cTotalDose = atoi (pDatadir.substr (pDatadir.find ("_") + 1, pDatadir.find ("kGy") - pDatadir.find ("_") ).c_str() );
    std::cout << pDatadir << " Chip id extracted " << cChipId << std::endl;
    std::cout << BOLDBLUE << "Plotting Bias sweep for " << pBias << " in data directory: " << pDatadir << RESET << std::endl;
    std::set<std::string> cSweeps{"VCth", "CAL_Vcasc", "VPLUS1", "VPLUS2", "VBGbias", "Ipa", "Ipre1", "Ipre2", "CAL_I", "Ibias", "Ipsf", "Ipaos", "Icomp", "Ihyst"};

    if (cSweeps.find (pBias) == std::end (cSweeps) ) std::cout << BOLDRED << "ERROR: " << pBias << " is not a sweep!" << RESET << std::endl;
    else
    {

        TMultiGraph* cGraph = new TMultiGraph();
        TLegend* cLegend = new TLegend (0.01, 0.01, 0.99, 0.99);
        cLegend->SetNColumns (2);
        cLegend->SetTextSize (0.03);
        std::vector<std::string> cFileList = list_folders (pDatadir, "Cbc3RadiationCycle.root");

        int cColor = 1;

        for (auto& cFilename : cFileList)
        {
            if (cFilename.find ("FULL") != std::string::npos)
            {
                TFile* cFile = TFile::Open (cFilename.c_str() );
                TDirectory* cDir = dynamic_cast< TDirectory* > ( gROOT->FindObject ("FE0CBC0") );


                if (cFile != nullptr && cDir != nullptr)
                {
                    for (auto cKey : *cDir->GetListOfKeys() )
                    {
                        std::string cGraphName = static_cast<std::string> (cKey->GetName() );

                        if (cGraphName.find (pBias + "_") != std::string::npos)
                        {
                            TGraph* cTmpGraph;// = static_cast<TGraph*> (cKey->ReadObj() );
                            cDir->GetObject (cGraphName.c_str(), cTmpGraph);
                            long int cTimestamp = atoi (cGraphName.substr (cGraphName.find ("TS") + 2).c_str() );

                            //this timestamp is in UTC, so add to it
                            if (gDaylightSavingTime) cTimestamp += (1 * 3600);

                            //else cTimestamp += 3600;

                            cTmpGraph->SetTitle (Form ("%3.0f %s", get_dose (pTimepair, cTimestamp), "kGy" ) );
                            float cDose = get_dose (pTimepair, cTimestamp);
                            float cPercentage = cDose / cTotalDose;

                            if (cPercentage <= 0.1)
                                cTmpGraph->SetLineColor (1);
                            else if (cPercentage > 0.1 && cPercentage <= 0.2)
                                cTmpGraph->SetLineColor (2);
                            else if (cPercentage > 0.2 && cPercentage <= 0.3)
                                cTmpGraph->SetLineColor (3);
                            else if (cPercentage > 0.3 && cPercentage <= 0.4)
                                cTmpGraph->SetLineColor (4);
                            else if (cPercentage > 0.4 && cPercentage <= 0.5)
                                cTmpGraph->SetLineColor (5);
                            else if (cPercentage > 0.5 && cPercentage <= 0.6)
                                cTmpGraph->SetLineColor (6);
                            else if (cPercentage > 0.6 && cPercentage <= 0.7)
                                cTmpGraph->SetLineColor (7);
                            else if (cPercentage > 0.7 && cPercentage <= 0.8)
                                cTmpGraph->SetLineColor (8);
                            else if (cPercentage > 0.8 && cPercentage <= 0.9)
                                cTmpGraph->SetLineColor (9);
                            else if (cPercentage > 0.9 && cPercentage <= 1)
                                cTmpGraph->SetLineColor (11);

                            cGraph->Add (cTmpGraph);
                            cLegend->AddEntry (cTmpGraph, Form ("%3.0f %s / %2.1fC", get_dose (pTimepair, cTimestamp), "kGy", get_temperature (Form ("TLog_chip%1d.txt", cChipId), cTimestamp) ), "l" );
                            //std::cout << cGraphName << " " << cKey->GetName() << std::endl;
                            cColor++;
                        }
                    }

                    cFile->Close();
                }
                else std::cout << BOLDRED << "ERROR, could not open File: " << cFilename << RESET << std::endl;
            }

        }

        TString cCanvasName = Form ("%s_%d", pBias.c_str(), gCanvasCounter);
        gCanvasCounter++;
        TCanvas* cCanvas = new TCanvas (cCanvasName, cCanvasName);
        cCanvas->cd();
        cGraph->Draw ("AL");
        cGraph->SetTitle (Form ("%s; DAC setting; DMM reading", pBias.c_str() ) );
        cCanvas->Modified();
        cCanvas->Update();
        pBias += "_sweep";
        cCanvas->SaveAs (create_filename (pDatadir, pBias ) );

        cCanvasName = Form ("%s", "Legend");
        TCanvas* cLegendCanvas = new TCanvas (cCanvasName, cCanvasName);
        cLegendCanvas->cd();
        cLegend->Draw();
        cLegendCanvas->SaveAs (create_filename (pDatadir, "Legend" ) );

    }
}

void plot_bias (std::string pDatadir, timepair pTimepair, std::string pBias, std::string pParameter)
{
    std::cout << BOLDBLUE << "Plotting " << pBias << " in data directory: " << pDatadir  << " against " << pParameter << RESET << std::endl;
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
                            if (gDaylightSavingTime) cTimestamp += (1 * 3600);

                            //else cTimestamp += 3600;

                            //std::cout << *cBias << " " << cTimestamp << " " << cValue << std::endl;
                            if (pParameter == "time")
                                cGraph->SetPoint (cGraph->GetN(), cTimestamp, cValue);
                            else if (pParameter == "dose")
                                cGraph->SetPoint (cGraph->GetN(), get_dose (pTimepair, cTimestamp), cValue);
                            else if (pParameter == "temperature")
                                cGraph->SetPoint (cGraph->GetN(), get_temperature (Form ("TLog_chip%1d.txt", cChipId), cTimestamp), cValue);
                        }
                    }

                    cFile->Close();
                }
                else std::cout << "error loading treee" << std::endl;
            }
        }

    }

    cGraph->GetYaxis()->SetTitleOffset (1.25);
    cGraph->SetMarkerStyle (8);
    cGraph->SetMarkerSize (.4);

    if (pBias == "MinimalPower")
    {
        cGraph->SetTitle ("Minimal Digital Current");
        cGraph->GetYaxis()->SetTitle ("Minimal Digital Current");
    }
    else
    {
        cGraph->GetYaxis()->SetTitle (pBias.c_str() );
        cGraph->SetTitle (pBias.c_str() );
    }

    if (pParameter == "time")
    {
        cGraph->GetXaxis()->SetTitle ("Time");
        draw_time (pDatadir, pTimepair, cGraph, cChipId);
    }
    else if (pParameter == "dose")
    {
        cGraph->GetXaxis()->SetTitle ("Dose [kGy]");
        draw_dose (pDatadir, cGraph);
    }
    else if (pParameter == "temperature")
    {
        //here i use draw_dose as this just plots the graph in it's own canvas
        cGraph->GetXaxis()->SetTitle ("Temperature [C]");
        draw_dose (pDatadir, cGraph);
    }
}

void analyze()
{
    //std::string cTimefile = "timefile_chip1";
    //std::string cDatadir = "Data/Chip1_358kGy";

    //std::string cTimefile = "timefile_chip0";
    //std::string cDatadir = "Data/Chip0_55kGy";

    //std::string cTimefile = "timefile_chip2";
    //std::string cDatadir = "Data/Chip2_42kGy";

    //std::string cTimefile = "timefile_chip3";
    //std::string cDatadir = "Data/Chip3_75kGy";

    //second irradiation

    //std::string cTimefile = "timefile_chip4";
    //std::string cDatadir = "Data/Chip4_76kGy";

    //std::string cTimefile = "timefile_chip5";
    //std::string cDatadir = "Data/Chip5_46kGy";

    //std::string cTimefile = "timefile_chip6";
    //std::string cDatadir = "Data/Chip6_51kGy";

    std::string cTimefile = "timefile_chip7";
    std::string cDatadir = "Data/Chip7_60kGy";

    //std::string cTimefile = "timefile_chip8";
    //std::string cDatadir = "Data/Chip8_80kGy";

    //std::string cTimefile = "timefile_chip9";
    //std::string cDatadir = "Data/Chip9_59kGy";

    std::vector<std::string> cSweeps{"VCth", "CAL_Vcasc", "VPLUS1", "VPLUS2", "VBGbias", "Ipa", "Ipre1", "Ipre2", "CAL_I", "Ipsf", "Ipaos", "Icomp", "Ihyst"};
    std::vector<std::string> cMeasurement{"VBG_LDO", "Vpafb", "Nc50", "VDDA", "MinimalPower", "Ibias"};

    timepair cTimepair = get_times (cTimefile);

    ////plot all the sweeps

    //for (auto sweep : cSweeps)
    //{
    //plot_sweep (cDatadir, cTimepair, sweep);
    //plot_bias (cDatadir, cTimepair, sweep, "time");
    //}

    //for (auto meas : cMeasurement)
    //plot_bias (cDatadir, cTimepair, meas, "time");

    //plot_pedenoise (cDatadir, cTimepair, "Pedestal", "time");
    //plot_pedenoise (cDatadir, cTimepair, "Noise", "time");
    //plot_lv (cDatadir, cTimepair, 4, 'I');


    //plot_scurves (cDatadir, cTimepair, 0);
    plot_scurves (cDatadir, cTimepair, 225);
}
#endif
