package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.AlreadyEditingException;

import java.util.ArrayList;
/*
*   This file is part of NSMB Editor 5.
*
*   NSMB Editor 5 is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   NSMB Editor 5 is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with NSMB Editor 5.  If not, see <http://www.gnu.org/licenses/>.
*/

    public abstract class FileWithLock extends DSFile
    {

		public FileWithLock() {}
		
		public FileWithLock(DSFilesystem parent, DSDirectory parentDir, String name, int id)
		{
			super(parent, parentDir, name, id);
		}

		// HANDLE EDITIONS

		// Invariants:
		// editedBy == null || editedIntervals.count == 0
		// No two intervals in editedIntervals have intersection
		
		protected Object editedBy;
		protected ArrayList<Interval> editedIntervals = new ArrayList<Interval>();
		
		protected class Interval {
			public int start, end;
			public Interval(int start, int end)
			{
				this.start = start;
				this.end = end;
			}
		}
		
		////
		
		@Override
        public void beginEdit(Object editor) throws AlreadyEditingException
        {
            if (editedBy != null || editedIntervals.size() != 0)
                throw new AlreadyEditingException(this);
 
            startEdition();
            editedBy = editor;
        }
		@Override
        public void endEdit(Object editor) throws Exception
        {
            if (editor == null || editor != editedBy)
                throw new Exception("Not correct editor: " + name());

            endEdition();
            editedBy = null;
        }
		@Override
        public void beginEditInterval(int start, int end) throws AlreadyEditingException, Exception
        {
        	validateInterval(start, end);
        	
            if (editedBy != null)
                throw new AlreadyEditingException(this);
			
        	for(Interval i : editedIntervals)
        	{
        		if(i.start < end && start < i.end)
        			throw new AlreadyEditingException(this);
        	}
            if (editedIntervals.size() == 0)
                startEdition();
            
            editedIntervals.add(new Interval(start, end));
        }
		
		@Override
        public void endEditInterval(int start, int end) throws Exception
        {
        	validateInterval(start, end);

        	Interval removeMe = null;
        	for(Interval i :  editedIntervals)
        	{
        		if(start==i.start&&end==i.end)
        			removeMe = i;
        	}
        	if(removeMe==null || !editedIntervals.remove(removeMe))
                throw new Exception("Not correct interval: " + name());

			if(editedIntervals.size() == 0)
				endEdition();
        }
        
        @Override
        public boolean beingEditedBy(Object ed)
        {
            return ed == editedBy;
        }
        
        protected boolean isAGoodEditor(Object editor)
        {
        	return editor == editedBy;
        }
        
        public void startEdition() {}
        public void endEdition() {}
    }
