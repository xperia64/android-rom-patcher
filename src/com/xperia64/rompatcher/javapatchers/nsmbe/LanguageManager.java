package com.xperia64.rompatcher.javapatchers.nsmbe;

import android.text.TextUtils;

import java.util.ArrayList;
import java.util.HashMap;

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


    public class LanguageManager {

        public final String tooltipIdentifier = ".tooltip";

        private static HashMap<String, HashMap<String, String>> Contents;
        private static HashMap<String, ArrayList<String>> ArrayLists;

        public static void Load(String[] LangFile) {
            Contents = new HashMap<String, HashMap<String, String>>();
            ArrayLists = new HashMap<String, ArrayList<String>>();

            boolean ArrayListMode = false;
            HashMap<String, String> CurrentSection = null;
            ArrayList<String> CurrentArrayList = null;

            for(String Line : LangFile) {
                String CheckLine = Line.trim();
                if (TextUtils.isEmpty(CheckLine)) continue;
                if (CheckLine.charAt(0) == ';') continue;

                if (CheckLine.startsWith("[") && CheckLine.endsWith("]")) {
                    String SectionName = CheckLine.substring(1, CheckLine.length() - 2);

                    if (SectionName.startsWith("LIST_")) {
                        CurrentArrayList = new ArrayList<String>();
                        ArrayLists.put(SectionName.substring(5), CurrentArrayList);
                        ArrayListMode = true;

                    } else {
                        if (Contents.containsKey(SectionName)) {
                            CurrentSection = Contents.get(SectionName);
                        } else {
                            CurrentSection = new HashMap<String, String>();
                            Contents.put(SectionName, CurrentSection);
                        }
                        ArrayListMode = false;
                    }

                } else {
                    if (ArrayListMode) {
                        CurrentArrayList.add(CheckLine);

                    } else {
                        if (CheckLine.contains("=")) {
                            int EqPos = CheckLine.indexOf('=');
                            CurrentSection.put(CheckLine.substring(0, EqPos), CheckLine.substring(EqPos + 1).replace("\\n", "\n"));
                        }
                    }
                }
            }
        }

        public static String Get(String Area, String Key) {
            if (Contents == null) return "<NOT LOADED>";

            if (Contents.containsKey(Area)) {
                HashMap<String, String> Referred = Contents.get(Area);
                if (Referred.containsKey(Key)) {
                    return Referred.get(Key);
                }
            }

            return String.format("<ERROR %s:%s>", Area, Key);
        }

        public static String Get(String Area, int Number)
        {
            if (Contents == null) return "<NOT LOADED>";

            if (Contents.containsKey(Area))
            {
                String[] keys = new String[Contents.get(Area).keySet().size()];
                int i = 0;
                for(String s : Contents.get(Area).keySet())
                {
                	keys[i] = s;
                	i++;
                }
                return (Contents.get(Area).get(keys[Number]));
            }

            return String.format("<ERROR %s:%s>", Area, Number);
        }

        public static ArrayList<String> GetArrayList(String Name) {
            if (ArrayLists == null){
            	ArrayList<String> dumbReturn = new ArrayList<String>();
            	dumbReturn.add( "<NOT LOADED>" );
            	return dumbReturn;
            }

            if (ArrayLists.containsKey(Name)) {
                return ArrayLists.get(Name);
            }
            ArrayList<String> anotherDumbReturn = new ArrayList<String>();
            anotherDumbReturn.add(String.format("<ERROR {0}>", Name));
            return anotherDumbReturn;
        }

        /*public static void ApplyToContainer(System.Windows.Forms.Control Container, String Area, System.Windows.Forms.ToolTip tooltip = null) {
            if (Contents == null) return;

            if (Contents.containsKey(Area)) {
                HashMap<String, String> Referred = Contents[Area];

                ApplyToContainer(Container, Referred, tooltip);

                if (Referred.containsKey("_TITLE")) {
                    Container.Text = Referred["_TITLE"];
                }

            } else {
                System.Windows.Forms.MessageBox.Show(
                    Area + " was missing from the language file.\nThe editor cannot continue.",
                    "NSMB Editor 4",
                    System.Windows.Forms.MessageBoxButtons.OK,
                    System.Windows.Forms.MessageBoxIcon.Exclamation);
            }
        }*/

        /*private static void ApplyToContainer(System.Windows.Forms.Control Container, HashMap<String, String> Referred, System.Windows.Forms.ToolTip tooltip = null) {
            foreach (System.Windows.Forms.Control Control in Container.Controls) {
                //Console.Out.WriteLine(Control.Name + " " + Control);
                if (Referred.containsKey(Control.Name)) {
                    Control.Text = Referred[Control.Name];
                }
                //Applies tooltip text to a control
                if (tooltip != null && Referred.containsKey(Control.Name + tooltipIdentifier)) {
                    tooltip.SetToolTip(Control, Referred[Control.Name + tooltipIdentifier]);
                }

                if (Control is System.Windows.Forms.ToolStrip) {
                    System.Windows.Forms.ToolStrip TS = Control as System.Windows.Forms.ToolStrip;
                    foreach (System.Windows.Forms.ToolStripItem TSItem in TS.Items) {
                        if (Referred.containsKey(TSItem.Name)) {
                            TSItem.Text = Referred[TSItem.Name];
                        }
                        //Sets tooltip on a toolstrip
                        if (Referred.containsKey(TSItem.Name + tooltipIdentifier)) {
                            TSItem.ToolTipText = Referred[TSItem.Name + tooltipIdentifier];
                        }
                    }
                }

                if (Control.Controls.Count > 0) {
                    ApplyToContainer(Control, Referred, tooltip);
                }
            }
        }*/
    }
