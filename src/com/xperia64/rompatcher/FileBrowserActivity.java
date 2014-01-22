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

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import android.app.ListActivity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.view.KeyEvent;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class FileBrowserActivity extends ListActivity {
	private List<String> item = null;
	private List<String> path = null;
	private String rootsd=Environment.getExternalStorageDirectory().getAbsolutePath();
	private String tmpitem="ROOT";
	private String extensions = "*.ips*.ups*.xdelta*.bps*.bsdiff*.ppf*";
	TextView myPath;


	@Override

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate( savedInstanceState );
		setContentView(R.layout.filebrowser);
		getDir(rootsd);
	}

	@Override
	public boolean onKeyDown(int KeyCode, KeyEvent event) {
		if(KeyCode == KeyEvent.KEYCODE_BACK){

			if(tmpitem.equals("ROOT")){
				this.finish();

			}else{
				getDir(tmpitem);

			}
			return true;
		}

		return super.onKeyDown(KeyCode, event);

	}



	public void getDir(String dirPath)
	{
		item = new ArrayList<String>();

		path = new ArrayList<String>();   
		File f = new File(dirPath);
		File[] files = f.listFiles(); 

		if (files.length>0){
			if(files != null)
			{
				Arrays.sort(files, new FileComparator());
			}

			if(!dirPath.equals("/"))
			{
				item.add("../");
				tmpitem=f.getParent();
				path.add(f.getParent());
			}else{tmpitem="ROOT";}



			for(int i=0; i < files.length; i++)

			{
				File file = files[i];

				if (files[i].isFile()){
					int dotPosition = files[i].getName().lastIndexOf(".");
					String extension="";
					if (dotPosition != -1) {
						extension = (files[i].getName().substring(dotPosition)).toLowerCase();
						if(extensions!=null)
						{
							if(extension!=null){

								if(extensions.contains("*"+extension+"*")&&(!Globals.mode)||((Globals.mode)&&!(extensions.contains("*"+extension+"*")))){

									path.add(file.getPath());
									item.add(file.getName());
								}
							}else if(files[i].getName().endsWith("/")){
								path.add(file.getPath()+"/");
								item.add(file.getName()+"/");}}
					}
				}else{

					path.add(file.getPath()+"/");

					item.add(file.getName()+"/");}}

		}else{
			if(!dirPath.equals("/")){
				item.add("../");
				tmpitem=f.getParent();
				path.add(f.getParent());

			}else{
				tmpitem="ROOT";
			}}
		ArrayAdapter<String> fileList =
				new ArrayAdapter<String>(this, R.layout.row, item);

		setListAdapter(fileList);

	}

	@Override

	protected void onListItemClick(ListView l, View v, int position, long id) {


		File file = new File(path.get(position));
		if (file.isDirectory())

		{

			if(file.canRead()){

				getDir(path.get(position));

			} else

			{

				new AlertDialog.Builder(this)

				.setIcon(R.drawable.icon)

				.setTitle("[" + file.getName() + "] "+getResources().getString(R.string.cantRead))

				.setPositiveButton("OK", 

						new DialogInterface.OnClickListener() {



					@Override

					public void onClick(DialogInterface dialog, int which) {


					}

				}).show();

			}

		}

		else

		{
			if(file.canRead()){
				if(Globals.mode)
				{
					Globals.fileToPatch=path.get(position);
					 Intent returnIntent = new Intent();
					 setResult(RESULT_OK,returnIntent);  
					 this.finish();
				}else{
					Globals.patchToApply=path.get(position);
					 Intent returnIntent = new Intent();
					 setResult(RESULT_OK,returnIntent);  
					 this.finish();
				}
			}

		}

	}

}




