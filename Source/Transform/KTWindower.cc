/*
 * KTWindower.cc
 *
 *  Created on: Apr 18, 2013
 *      Author: nsoblath
 */

#include "KTWindower.hh"

#include "KTEggHeader.hh"
#include "KTTimeSeriesData.hh"
#include "KTTimeSeriesFFTW.hh"
#include "KTTimeSeriesReal.hh"
#include "KTWindowFunction.hh"

#include "factory.hh"

#include <cmath>

using std::string;


namespace Katydid
{
    KTLOGGER(windowlog, "KTWindower");

    KT_REGISTER_PROCESSOR(KTWindower, "windower");

    KTWindower::KTWindower(const std::string& name) :
            KTProcessor(name),
            fWindowFunction(NULL),
            fWindowed("windowed", this),
            fHeaderSlot("header", this, &KTWindower::InitializeWithHeader),
            fTimeSeriesFFTWSlot("ts-fftw", this, &KTWindower::WindowDataFFTW, &fWindowed),
            fTimeSeriesRealSlot("ts-real", this, &KTWindower::WindowDataReal, &fWindowed)
    {
    }

    KTWindower::~KTWindower()
    {
        delete fWindowFunction;
    }

    bool KTWindower::Configure(const scarab::param_node* node)
    {
        // Config-file settings
        if (node == NULL) return true;

        string windowType = node->get_value("window-function-type", "rectangular");
        if (! SelectWindowFunction(windowType))
        {
            return false;
        }

        if (! fWindowFunction->Configure(node->node_at("window-function")))
        {
            return false;
        }

        return true;
    }

    bool KTWindower::SelectWindowFunction(const string& windowType)
    {
        KTWindowFunction* tempWF = scarab::factory< KTWindowFunction >::get_instance()->create(windowType);
        if (tempWF == NULL)
        {
            KTERROR(windowlog, "Invalid window function type given: <" << windowType << ">.");
            return false;
        }
        SetWindowFunction(tempWF);
        return true;
    }

    bool KTWindower::InitializeWindow(double binWidth, double size)
    {
        fWindowFunction->SetBinWidth(binWidth);
        fWindowFunction->SetSize(size);
        fWindowFunction->RebuildWindowFunction();
        return true;
    }

    bool KTWindower::InitializeWithHeader(KTEggHeader& header)
    {
        if (! InitializeWindow(1. / header.GetAcquisitionRate(), header.GetChannelHeader(0)->GetSliceSize()))
        {
            KTERROR(windowlog, "Something went wrong while initializing the window function!");
            return false;
        }
        KTDEBUG(windowlog, "Window function initialized with header");
        return true;
    }

    bool KTWindower::WindowDataReal(KTTimeSeriesData& tsData)
    {
        if (tsData.GetTimeSeries(0)->GetNTimeBins() != fWindowFunction->GetSize())
        {
            fWindowFunction->AdaptTo(&tsData); // this call rebuilds the window, so that doesn't need to be done separately
        }

        unsigned nComponents = tsData.GetNComponents();

        for (unsigned iComponent = 0; iComponent < nComponents; ++iComponent)
        {
            KTTimeSeriesReal* nextInput = dynamic_cast< KTTimeSeriesReal* >(tsData.GetTimeSeries(iComponent));
            if (nextInput == NULL)
            {
                KTERROR(windowlog, "Incorrect time series type: time series did not cast to KTTimeSeriesFFTW.");
                return false;
            }

            bool result = ApplyWindow(nextInput);

            if (! result)
            {
                KTERROR(windowlog, "Component <" << iComponent << "> did not get windowed correctly.");
                return false;
            }
        }

        KTINFO(windowlog, "Windowing complete");

        return true;
    }

    bool KTWindower::WindowDataFFTW(KTTimeSeriesData& tsData)
    {
        if (tsData.GetTimeSeries(0)->GetNTimeBins() != fWindowFunction->GetSize())
        {
            fWindowFunction->AdaptTo(&tsData); // this call rebuilds the window, so that doesn't need to be done separately
        }

        unsigned nComponents = tsData.GetNComponents();

        for (unsigned iComponent = 0; iComponent < nComponents; ++iComponent)
        {
            KTTimeSeriesFFTW* nextInput = dynamic_cast< KTTimeSeriesFFTW* >(tsData.GetTimeSeries(iComponent));
            if (nextInput == NULL)
            {
                KTERROR(windowlog, "Incorrect time series type: time series did not cast to KTTimeSeriesFFTW.");
                return false;
            }

            bool result = ApplyWindow(nextInput);

            if (! result)
            {
                KTERROR(windowlog, "Component <" << iComponent << "> did not get windowed correctly.");
                return false;
            }
        }

        KTINFO(windowlog, "Windowing complete______________________________________________");

        return true;
    }

    bool KTWindower::ApplyWindow(KTTimeSeriesReal* ts) const
    {
        unsigned nBins = ts->size();
        if (nBins != fWindowFunction->GetSize())
        {
            KTWARN(windowlog, "Number of bins in the data provided does not match the number of bins set for this window\n"
                    << "   Bin expected: " << fWindowFunction->GetSize() << ";   Bins in data: " << nBins);
            return false;
        }

        for (unsigned iBin=0; iBin < nBins; ++iBin)
        {
            (*ts)(iBin) = (*ts)(iBin) * fWindowFunction->GetWeight(iBin);
        }

        return true;
    }

    bool KTWindower::ApplyWindow(KTTimeSeriesFFTW* ts) const
    {
        unsigned nBins = ts->size();
        if (nBins != fWindowFunction->GetSize())
        {
            KTWARN(windowlog, "Number of bins in the data provided does not match the number of bins set for this window\n"
                    << "   Bin expected: " << fWindowFunction->GetSize() << ";   Bins in data: " << nBins);
            return false;
        }
	
	KTINFO(windowlog, "It's working aaaaaaaaaaaa _____________________________ :)");

        double weight;
	double S2_factor;
	double scale;

	for (unsigned iBin=0; iBin < nBins; ++iBin)
        {
            weight = fWindowFunction->GetWeight(iBin);
            S2_factor += weight * weight;
        }

	scale = sqrt(1/S2_factor);

        for (unsigned iBin=0; iBin < nBins; ++iBin)
        {
            weight = fWindowFunction->GetWeight(iBin);
            ts->SetRect(iBin, ts->GetReal(iBin)*weight*scale, ts->GetImag(iBin)*weight*scale);
        }

	KTINFO(windowlog, "S2_factor is ________________" << S2_factor);

        return true;
    }

    void KTWindower::SetWindowFunction(KTWindowFunction* wf)
    {
        delete fWindowFunction;
        fWindowFunction = wf;
        return;
    }



} /* namespace Katydid */
