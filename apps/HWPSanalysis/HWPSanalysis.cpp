#include <cstdio>

#include "MarSystemManager.h"
#include "AudioSink.h"
#include "SoundFileSink.h"
#include "SoundFileSource.h"
#include "Gain.h"
#include "Messager.h"
#include "Conversions.h"
#include "CommandLineOptions.h"
#include "PeClusters.h"
#include "PeUtilities.h"

#include <string>

using namespace std;
using namespace Marsyas;


string pluginName = EMPTYSTRING;
string inputDirectoryName = EMPTYSTRING;
string outputDirectoryName = EMPTYSTRING;
string fileName = EMPTYSTRING;
string noiseName = EMPTYSTRING;
string fileResName = EMPTYSTRING;
string filePeakName = EMPTYSTRING;
string panningInfo = EMPTYSTRING;
string intervalFrequency = "50-4000";
string harmonizeFileName = EMPTYSTRING;

// Global variables for command-line options 
bool helpopt_ = 0;
bool usageopt_ =0;
bool peakStore_=0;
int fftSize_ = 2048;
int winSize_ = 2048;
// if kept the same no time expansion
int hopSize_ = 512;
// nb Sines
int nbSines_ = 50;
// nbClusters
int nbClusters_ = 3;
// output buffer Size
int bopt_ = 128;
// output gain
mrs_real gopt_ = 1.0;
// number of accumulated frames
mrs_natural accSize_ = 1;
// type of similarity Metrics
string similarityType_ = "ho";
// store for clustered peaks
realvec peakSet_;
// delay for noise insertion
mrs_real noiseDelay_=0;
// gain for noise insertion
mrs_real noiseGain_=.1;
// duration for noise insertion
mrs_real noiseDuration_=0;
// sampling frequency
mrs_real samplingFrequency_=1;

bool microphone_ = false;
bool analyse_ = false;
mrs_natural synthetize_ = -1;

CommandLineOptions cmd_options;

void 
printUsage(string progName)
{
	MRSDIAG("HWPSanalysis.cpp - printUsage");
	cerr << "Usage : " << progName << " [file]" << endl;
	cerr << endl;
	cerr << "If no filename is given the default live audio input is used. " << endl;
}

void 
printHelp(string progName)
{
	MRSDIAG("HWPSanalysis.cpp - printHelp");
	cerr << "HWPSanalysis, MARSYAS, Copyright Mathieu Lagrange " << endl;
	cerr << "report bugs to lagrange@uvic.ca" << endl;
	cerr << "--------------------------------------------" << endl;
	cerr << "Usage : " << progName << " [file]" << endl;
	cerr << endl;
	cerr << "if no filename is given the default live audio input is used. " << endl;
	cerr << "Options:" << endl;
	cerr << "-n --fftsize         : size of fft " << endl;
	cerr << "-w --winsize         : size of window " << endl;
	cerr << "-s --sinusoids       : number of sinusoids per frame" << endl;
	cerr << "-b --buffersize      : audio buffer size" << endl;
	cerr << "-o --outputdirectoryname   : output directory path" << endl;
	cerr << "-p --panning : panning informations <foreground level (0..1)>-<foreground pan (-1..1)>-<background level>-<background pan> " << endl;
	cerr << "-S --synthetise : synthetize using an oscillator bank (0), an IFFT mono (1), or an IFFT stereo (2)" << endl;
	cerr << "-H --harmonize : change the frequency accoring to the file provided" << endl;	
	cerr << "" << endl;
	cerr << "-u --usage           : display short usage info" << endl;
	cerr << "-h --help            : display this information " << endl;


	exit(1);
}


// original monophonic PeVocoding
void
HWPSanalyse(string sfName, string outsfname, mrs_natural N, mrs_natural Nw, 
						mrs_natural D, mrs_natural S, mrs_natural synthetize)
{
	mrs_natural nbFrames_=0, harmonize_=0;
	realvec harmonizeData_;
	MarControlPtr ctrl_harmonize_;
	cout << "Extracting Peaks and Clusters" << endl;
	MarSystemManager mng;

	// create the phasevocoder network
	MarSystem* pvseries = mng.create("Series", "pvseries");

	if(analyse_)
	{
		// add original source 
		if (microphone_) 
			pvseries->addMarSystem(mng.create("AudioSource", "src"));
		else 
			pvseries->addMarSystem(mng.create("SoundFileSource", "src"));
		
		
		//create analyzer (using composite prototype)
		pvseries->addMarSystem(mng.create("HWPSspectrumnet", "WHASP"));
		pvseries->linkctrl("mrs_natural/Sinusoids","HWPSspectrumnet/WHASP/PeAnalyse/analyse/mrs_natural/Sinusoids");
		
		////////////////////////////////////////////////////////////////
		// update the controls
		////////////////////////////////////////////////////////////////
		if (microphone_) 
		{
			pvseries->updctrl("mrs_natural/inSamples", D);
			pvseries->updctrl("mrs_natural/inObservations", 1);
		}
		else
		{
// 			pvseries->linkctrl("HWPSspectrumnet/WHASP/mrs_natural/pos", "SoundFileSource/src/mrs_natural/pos");  //!!!!!!! [!]
// 			pvseries->linkctrl("HWPSspectrumnet/WHASP/mrs_string/filename", "SoundFileSource/src/mrs_string/filename"); //!!!!!!!!! [!]
// 			pvseries->linkctrl("HWPSspectrumnet/WHASP/mrs_bool/notEmpty", "SoundFileSource/src/mrs_bool/notEmpty"); //!!!!!!!! [!]
			
			pvseries->updctrl("SoundFileSource/src/mrs_string/filename", sfName);
			pvseries->updctrl("mrs_natural/inSamples", D);
			pvseries->updctrl("mrs_natural/inObservations", 1);
			samplingFrequency_ = pvseries->getctrl("SoundFileSource/src/mrs_real/osrate")->toReal();
		}


		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/mrs_natural/Decimation", D);
		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/mrs_natural/WindowSize", Nw+1);
		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/mrs_natural/FFTSize", N);
		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/mrs_string/WindowType", "Hanning");
		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/mrs_natural/zeroPhasing", 1);
		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/Shifter/sh/mrs_natural/shift", 1);
		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/mrs_natural/Decimation", D);      
		pvseries->updctrl("mrs_natural/Sinusoids", S);  
		pvseries->updctrl("HWPSspectrumnet/WHASP/PeAnalyse/analyse/PeConvert/conv/mrs_natural/nbFramesSkipped", (N/D));  
	}
	else
	{
		// create realvecSource
		MarSystem *peSource = mng.create("RealvecSource", "peSource");
		pvseries->addMarSystem(peSource);
		/*

		mrs_natural nbF_=0;
		realvec peakSet_;
		peakSet_.read(sfName);

		MATLAB_PUT(peakSet_, "peaks");
		MATLAB_EVAL("plotPeaks(peaks)");

		for (mrs_natural i=0 ; i<peakSet_.getRows() ; i++)
		if(peakSet_(i, pkTime)>nbF_)
		{
		nbF_ = peakSet_(i, pkTime);
		}
		nbF_++;

		realvec peakSetV_(nbSines_*nbPkParameters, nbF_);
		peakSetV_.setval(0);
		peaks2V(peakSet_, peakSetV_, peakSetV_, nbSines_);

		*/
		realvec peakSet_;
		mrs_real fs;
		mrs_natural nbFrames;
		peakLoad(peakSet_, sfName, fs, S, nbFrames, D);
		pvseries->setctrl("RealvecSource/peSource/mrs_realvec/data", peakSet_);
		pvseries->setctrl("mrs_real/israte", fs);
	}

	if(peakStore_)
	{
		// realvec sink to store peaks
		MarSystem *peSink = mng.create("RealvecSink", "peSink");
		pvseries->addMarSystem(peSink);
	}

	if(harmonizeFileName != "MARSYAS_EMPTY")
	{
		harmonizeData_.read(harmonizeFileName);
		if(!harmonizeData_.getSize())
			cout << "Unable to open "<< harmonizeFileName << endl;
		harmonize_=1;


		// ctrl_harmonize_->setValue(0, 0.);
		synthetize = 3;
	}

	if(synthetize>-1 )
	{
		synthNetCreate(&mng, outsfname, microphone_, synthetize);
		MarSystem *peSynth = mng.create("PeSynthetize", "synthNet");
		pvseries->addMarSystem(peSynth);
		synthNetConfigure (pvseries, sfName, outsfname, fileResName, panningInfo, 1, Nw, D, S, 1, microphone_, synthetize, bopt_, Nw+1-D);
	}

	if(harmonize_)
	{
		ctrl_harmonize_= pvseries->getctrl("PeSynthetize/synthNet/Series/postNet/PeSynOscBank/pso/mrs_realvec/harmonize");
		ctrl_harmonize_->stretch(harmonizeData_.getCols());
	}

	mrs_real globalSnr = 0;

	mrs_natural nb=0;

	//	mrs_real time=0;
	if(analyse_ || synthetize > -1)
		while(1)
		{
			pvseries->tick();
			if (harmonize_) 
			{

				for (mrs_natural i=0 ; i<harmonizeData_.getCols() ; i++)
					//ctrl_harmonize_->setValue(i, 0.0);
					//ctrl_harmonize_->setValue(1, 1.0);
					//ctrl_harmonize_->setValue(2, 0.1);
					if (harmonizeData_.getRows() > nbFrames_)
						ctrl_harmonize_->setValue(i, harmonizeData_(nbFrames_, i));
					else 
					{
						ctrl_harmonize_->setValue(i, 0);
						cout << "Harmonize file too short" << endl;
					}
			}
			nbFrames_++;
			if (!microphone_)
			{
				bool temp;
				if(analyse_)
				{
					temp = pvseries->getctrl("SoundFileSource/src/mrs_bool/notEmpty")->toBool();
					mrs_real timeRead =  pvseries->getctrl("SoundFileSource/src/mrs_natural/pos")->toNatural()/samplingFrequency_;
					mrs_real timeLeft =  pvseries->getctrl("SoundFileSource/src/mrs_natural/size")->toNatural()/samplingFrequency_;
					printf("%.2f / %.2f \r", timeRead, timeLeft);
				}
				else 
					temp =	!pvseries->getctrl("RealvecSource/peSource/mrs_bool/done")->toBool();

				///*bool*/ temp = pvseries->getctrl("PeAnalyse/peA/SoundFileSource/src/mrs_bool/notEmpty")->toBool();
				if (temp == false)
					break;
			}
		}

		if(peakStore_)
		{
			realvec vec = pvseries->getctrl("RealvecSink/peSink/mrs_realvec/data")->toVec();
			peakStore(vec, filePeakName, samplingFrequency_, D); 

			MATLAB_PUT(peakSet_, "peaks");
			MATLAB_EVAL("plotPeaks(peaks)");

			realvec realTry(nbFrames_, 5);
			realTry.setval(0);
			for (mrs_natural i=0 ; i<nbFrames_ ; i++)
			{
				realTry(i, 1) = 20;
				realTry(i, 2) = .8;
				realTry(i, 3) = .25;
				realTry(i, 4) = .6;
			}
			ofstream tryFile;
			string harmonizeName = filePeakName+"HarmoStream";
			tryFile.open(harmonizeName.c_str());
			tryFile<< realTry;
		}
}

void 
initOptions()
{
	cmd_options.addBoolOption("help", "h", false);
	cmd_options.addBoolOption("usage", "u", false);
	cmd_options.addNaturalOption("voices", "v", 1);
	cmd_options.addStringOption("filename", "f", EMPTYSTRING);
	cmd_options.addStringOption("outputdirectoryname", "o", EMPTYSTRING);
	cmd_options.addStringOption("inputdirectoryname", "I", EMPTYSTRING);
	cmd_options.addNaturalOption("winsize", "w", winSize_);
	cmd_options.addNaturalOption("fftsize", "n", fftSize_);
	cmd_options.addNaturalOption("sinusoids", "s", nbSines_);
	cmd_options.addNaturalOption("bufferSize", "b", bopt_);
	cmd_options.addStringOption("intervalFrequency", "i", intervalFrequency);
	cmd_options.addStringOption("panning", "p", EMPTYSTRING);
	cmd_options.addStringOption("Harmonize", "H", EMPTYSTRING);
	cmd_options.addNaturalOption("synthetize", "S", synthetize_);
	cmd_options.addBoolOption("analyse", "A", analyse_);
	cmd_options.addBoolOption("PeakStore", "P", peakStore_);
}

void 
loadOptions()
{
	helpopt_ = cmd_options.getBoolOption("help");
	usageopt_ = cmd_options.getBoolOption("usage");
	fileName   = cmd_options.getStringOption("filename");
	inputDirectoryName = cmd_options.getStringOption("inputdirectoryname");
	outputDirectoryName = cmd_options.getStringOption("outputdirectoryname");
	winSize_ = cmd_options.getNaturalOption("winsize");
	fftSize_ = cmd_options.getNaturalOption("fftsize");
	nbSines_ = cmd_options.getNaturalOption("sinusoids");
	bopt_ = cmd_options.getNaturalOption("bufferSize");
	intervalFrequency = cmd_options.getStringOption("intervalFrequency");
	panningInfo = cmd_options.getStringOption("panning");
	harmonizeFileName = cmd_options.getStringOption("Harmonize");
	synthetize_ = cmd_options.getNaturalOption("synthetize");
	analyse_ = cmd_options.getBoolOption("analyse");
	peakStore_ = cmd_options.getBoolOption("PeakStore");
}

int
main(int argc, const char **argv)
{
	MRSDIAG("HWPSanalysis.cpp - main");

	initOptions();
	cmd_options.readOptions(argc, argv);
	loadOptions();  

	vector<string> soundfiles = cmd_options.getRemaining();
	vector<string>::iterator sfi;

	string progName = argv[0];  

	if (helpopt_) 
		printHelp(progName);

	if (usageopt_)
		printUsage(progName);


	cerr << "HWPSanalysis configuration (-h show the options): " << endl;
	cerr << "fft size (-n)      = " << fftSize_ << endl;
	cerr << "win size (-w)      = " << winSize_ << endl;
	cerr << "sinusoids (-s)     = " << nbSines_ << endl;
	cerr << "outFile  (-f)      = " << fileName << endl;
	cerr << "outputDirectory  (-o) = " << outputDirectoryName << endl;
	cerr << "inputDirectory  (-i) = " << inputDirectoryName << endl;

	// soundfile input 
	string sfname;
	if (soundfiles.size() != 0)   
	{
		// process several soundFiles
		for (sfi=soundfiles.begin() ; sfi!=soundfiles.end() ; sfi++)
		{
			FileName Sfname(*sfi);
			/*	if(outputDirectoryName == EMPTYSTRING)
			{
			outputDirectoryName = ".";
			}*/

			if(Sfname.ext() == "peak")
			{
				analyse_ = 0;
				if(synthetize_ == -1)
					synthetize_ = 0;
				peakStore_=0;
			}
			if(Sfname.ext() == "wav")
			{
				analyse_ = 1;
			}

			string path;
			if(outputDirectoryName != EMPTYSTRING)
				path = outputDirectoryName;
			else
				path =Sfname.path();

			fileName = path + "/" + Sfname.nameNoExt() + "Syn.wav" ;
			filePeakName = path + "/" + Sfname.nameNoExt() + ".peak" ;
			cout << "HWPSanalysis " << Sfname.name() << endl; 

			HWPSanalyse(*sfi, fileName, fftSize_, winSize_, hopSize_, nbSines_, synthetize_);
		}
	}
	else
	{
		cout << "Using live microphone input" << endl;
		microphone_ = true;
		HWPSanalyse("microphone", fileName, fftSize_, winSize_, hopSize_, nbSines_, synthetize_);
	}



	exit(0);
}


