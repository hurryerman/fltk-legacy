//
// "$Id: Fl_Float_Input.h,v 1.3 2002/01/23 08:46:00 spitzak Exp $"
//
// Floating point input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-1999 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@easysw.com".
//

#ifndef Fl_Float_Input_H
#define Fl_Float_Input_H

#include "Fl_Numeric_Input.h"

class FL_API Fl_Float_Input : public Fl_Numeric_Input {
  virtual bool replace(int, int, const char*, int);
public:
  enum {FLOAT = 0, INT = 1};
  Fl_Float_Input(int x,int y,int w,int h,const char *l = 0)
    : Fl_Numeric_Input(x,y,w,h,l) {}
};

#endif

//
// End of "$Id: Fl_Float_Input.h,v 1.3 2002/01/23 08:46:00 spitzak Exp $".
//
