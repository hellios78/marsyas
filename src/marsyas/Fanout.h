/*
** Copyright (C) 1998-2006 George Tzanetakis <gtzan@cs.uvic.ca>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef MARSYAS_FANOUT_H
#define MARSYAS_FANOUT_H

#include "MarSystem.h"

namespace Marsyas
{
/** 
\class Fanout
\ingroup Composites
\brief Fanout of MarSystem objects

All the children Marsystems of this composite receive the same 
input and their outputs are stacked as rows of observations. 

Controls:
- \b mrs_natural/disable [w] : passes in the number of the child
  MarSystem to disable.
- \b mrs_natural/enable	[w] : pass in the number of the child MarSystem to
  enable.  (by default all children are enabled)
- \b mrs_natural/enableChild	[w] : pass in the type/name of the child MarSystem to
  enable.  (by default all children are enabled)
- \b mrs_natural/disableChild	[w] : pass in the type/name of the child MarSystem to
  disable.
*/


class Fanout: public MarSystem
{
private:
  void addControls();
  void myUpdate(MarControlPtr sender);
  void deleteSlices();  

  realvec enabled_;
  realvec localIndices_;
  
  mrs_natural enable_, enableChildIndex_;
  mrs_natural disable_, disableChildIndex_;
  
  mrs_string enableChild_;
  mrs_string disableChild_;
  
  
  
public:
  Fanout(std::string name);
  ~Fanout();
  MarSystem* clone() const;    
 
  void myProcess(realvec& in, realvec& out);
};

}//namespace Marsyas

#endif

	

	
