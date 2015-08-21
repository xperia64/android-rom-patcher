package com.xperia64.rompatcher.javapatchers.nsmbe;

/*import com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem.DSFile;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;
import java.util.ArrayList;*/

//using System;
//using System.Collections.Generic;
//using System.Text;


// This doesn't seem to be used. It's even dummy'd out.
    public class ROMUserInfo
    {
    	/*
        //public String FilePath;

        public HashMap<Integer, ArrayList<String>> descriptions = new HashMap<Integer, ArrayList<String>>();
        public HashMap<String, HashMap<Integer, String>> lists = new HashMap<String, HashMap<Integer, String>>();

        
        public static boolean tryParse(String s, int num)
        {
        	 try {
        		    num = Integer.parseInt(s);
        		  } catch (NumberFormatException e) {
        		    return false;
        		  }
        	 return true;
        }
        public ROMUserInfo(String ROMPath)
        {
            //FilePath = ROMPath.SubString(0, ROMPath.LastIndexOf('.') + 1) + "txt";
            int lineNum = 0;
                //if (!System.IO.File.Exists(FilePath)) return;
                //System.IO.StreamReader s = new System.IO.StreamReader(FilePath);
                DSFile file = ROM.FS.getDSFileByName("00DUMMY");
                String[] lines = null;
				try {
					lines = new String(file.getContents(), "US-ASCII").split("\n");
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
                ArrayList<String> curArrayList = null;
                HashMap<Integer, String> curDict = null;
                boolean readdescriptions = false;
                for (lineNum = 0; lineNum < lines.length; lineNum++)
                {
                    String line = lines[lineNum];
                    if (line != "")
                    {
                        if (line.startsWith("["))
                        {
                            line = line.substring(1, line.length() - 2);
                            int num = 0;
                            if (tryParse(line, num)) {
                                readdescriptions = true;
                                curArrayList = new ArrayList<String>();
                                for (int l = 0; l < 256; l++)
                                    curArrayList.add("");
                                descriptions.put(num, curArrayList);
                            } else {
                                readdescriptions = false;
                                curDict = new HashMap<Integer,String>();
                                lists.put(line, curDict);
                            }
                        }
                        else if (curArrayList != null || curDict != null) {
                            int num = Integer.parseInt(line.substring(0, line.indexOf("=")).trim());
                            String name = line.substring(line.indexOf("=") + 1).trim();
                            if (readdescriptions) {
                                if (num < 256)
                                    curArrayList.set(num, name);
                            } else
                                curDict.put(num, name);
                        }
                    }
                }
            }
           
                //System.Windows.Forms.MessageBox.Show(String.Format(LanguageManager.Get("ROMUserInfo", "ErrorRead"), lineNum + 1, ex.Message));

        

        public ArrayList<String> getFullArrayList(String name)
        {
            ArrayList<String> fullArrayList = LanguageManager.GetArrayList(name);
            ArrayList<String> newArrayList = new ArrayList<String>();
            for(String i : fullArrayList)
                newArrayList.add(i);
            if (lists.containsKey(name))
            {
            	HashMap<Integer, String> tmp = lists.get(name);
            	for(Integer i : tmp.keySet())
            	{
            		if(newArrayList.size() > i)
            		{
            			newArrayList.set(i, tmp.get(i));
            		}
            	}
            }
                
            if (name == "Music")
                for (int l = 0; l < ROM.MusicNumbers.length; l++)
                    newArrayList.set(l, String.format("%02x: %s", ROM.MusicNumbers[l], newArrayList.get(l)));

            return newArrayList;
        }

        public void setArrayListItem(String listName, int num, String value, boolean rewriteFile)
        {
            if (!lists.containsKey(listName))
                lists.put(listName, new HashMap<Integer, String>());
            if (lists.get(listName).containsKey(num))
                lists.get(listName).put(num, value);
            else
                lists.get(listName).put(num, value);
            if (rewriteFile)
                SaveFile();
        }

        public void removeArrayListItem(String listName, int num, boolean rewriteFile)
        {
            if (!lists.containsKey(listName)) return;
            lists.get(listName).remove(num);
            if (lists.get(listName).size() == 0)
                lists.remove(listName);
            if (rewriteFile)
                SaveFile();
        }

        public void createDescriptions(int tilesetNum)
        {
            ArrayList<String> descArrayList = new ArrayList<String>();
            for (int l = 0; l < 256; l++)
                descArrayList.add("");
            if (tilesetNum == 65535) {
                ArrayList<String> defaults = LanguageManager.GetArrayList("ObjNotes");
                for (int l = 0; l < defaults.size(); l++) {
                    int idx = defaults.get(l).indexOf("=");
                    int num = 0;
                    if (tryParse(defaults.get(l).substring(0, idx), num))
                        descArrayList.set(num, defaults.get(l).substring(idx + 1));
                }
            }
            descriptions.put(tilesetNum, descArrayList);
        }

        public void SaveFile()
        {
            try
            {
                StringBuilder text = new StringBuilder();
                //System.IO.StreamWriter s = new System.IO.StreamWriter(new System.IO.FileStream(FilePath, System.IO.FileMode.Create, System.IO.FileAccess.Write));
                // Write lists
                
                
                /*foreach (KeyValuePair<String, HashMap<Integer, String>> list in lists)
                {
                    text += "[" + list.Key + "]\n";
                    foreach (KeyValuePair<Integer, String> item in list.Value)
                        text += item.Key.ToString() + "=" + item.Value + "\n";
                }*/
                /*
                for(String s : lists.keySet())
                {
                	text.append("[" + s + "]\n");
                	HashMap<Integer, String> tmp = lists.get(s);
                	for(Integer i : tmp.keySet())
                	{
                		text.append(i.toString() + "=" + tmp.get(i) + "\n");
                	}
                }
                
                
                for(Integer s : descriptions.keySet())
                {
                	int num = 0;
                     text.append("[" + s.toString() + "]\n");
                     ArrayList<String> tmp = descriptions.get(s);
                     for(String desc : tmp) {
                         if (!(desc.isEmpty()))
                             text.append(Integer.toString(num) + "=" + desc + "\n");
                         num++;
                     }
                }
                
                // Write descriptions
                /*foreach (KeyValuePair<Integer, ArrayList<String>> item in descriptions)
                {
                    int num = 0;
                    text += "[" + item.Key.ToString() + "]\n";
                    foreach (String desc in item.Value) {
                        if (desc != String.Empty)
                            text += num.ToString() + "=" + desc + "\n";
                        num++;
                    }
                }*//*
                DSFile file = ROM.FS.getDSFileByName("00DUMMY");
                file.beginEdit(this);
                file.replace(text.toString().getBytes("US-ASCII"), this);
                file.endEdit(this);
            }
            catch (Exception ex)
            {
                //System.Windows.Forms.MessageBox.Show(String.Format(LanguageManager.Get("ROMUserInfo", "ErrorWrite"), ex.Message));
                System.out.println("ROMUserInfo Exception: "+ex.getMessage());
            }
        }*/
    }
