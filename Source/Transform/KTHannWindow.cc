/*
 * KTHannWindow.cc
 *
 *  Created on: Sep 18, 2011
 *      Author: nsoblath
 */

#include "KTHannWindow.hh"

#include "KTLogger.hh"
#include "KTMath.hh"

#include <cmath>

using std::string;

namespace Katydid
{
    KT_REGISTER_WINDOWFUNCTION(KTHannWindow, "hann")

    KTLOGGER(windowlog, "KTHannWindow");

    KTHannWindow::KTHannWindow(const string& name) :
            KTWindowFunction(name),
	    fNormalized(false)
    {
    }

    KTHannWindow::~KTHannWindow()
    {
    }

    bool KTHannWindow::ConfigureWFSubclass(const scarab::param_node* node)
    {
	SetNormalized(node->get_value< bool >("normalized", fNormalized));

        KTDEBUG(windowlog, "Hann WF configured");
        return true;
    }

    double KTHannWindow::GetWeight(double time) const
    {
        return GetWeight(KTMath::Nint(time / fBinWidth));
    }

    void KTHannWindow::RebuildWindowFunction()
    {
        fWindowFunction.resize(fSize);
        double twoPiOverNBinsMinus1 = KTMath::TwoPi() / (double)(fSize - 1);
        
	if (fNormalized)
	{
	      for (unsigned iBin=0; iBin<fSize; iBin++)
        	{
             	   fWindowFunction[iBin] = (1. - cos((double)iBin * twoPiOverNBinsMinus1));
        	}
	
	} else {
	
              for (unsigned iBin=0; iBin<fSize; iBin++)
        	{
	           fWindowFunction[iBin] = 0.5 * (1. - cos((double)iBin * twoPiOverNBinsMinus1));
        	}
	}

	return;
    }

} /* namespace Katydid */
