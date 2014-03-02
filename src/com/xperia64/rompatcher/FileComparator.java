/*******************************************************************************
 * This file is part of ROM Patcher.
 * 
 * Copyright (c) 2014 xperia64.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 * 	Paul Kratt - main MultiPatch application for Mac OSX
 * 	byuu - UPS and BPS patchers
 * 	Neil Corlett - IPS patcher
 * 	Daniel Ekstr'm - PPF patcher
 * 	Josh MacDonald - XDelta
 * 	Colin Percival - BSDiff
 * 	xperia64 - port to Android and IPS32 support
 ******************************************************************************/
package com.xperia64.rompatcher;

import android.text.TextUtils;

import java.io.File;
import java.text.Collator;
import java.util.Comparator;


 public class FileComparator implements Comparator<Object>
	  {
	  private Collator c = Collator.getInstance();

	  public int compare(Object o1,
	                     Object o2)
	  {
	    if((o1 == o2)||(TextUtils.isEmpty(((File)o1).getName())||(TextUtils.isEmpty(((File)o2).getName()))))
	      return 0;

	    File f1 = (File) o1;
	    File f2 = (File) o2;

	    if(f1.isDirectory() && f2.isFile())
	      return -1;
	    if(f1.isFile() && f2.isDirectory())
	      return 1;
	    
	    return c.compare(f1.getName(), f2.getName());
	  }
	  }
