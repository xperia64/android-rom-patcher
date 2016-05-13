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
import java.util.Locale;

import android.app.ListActivity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.text.TextUtils;
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
	private String extensions = "*.aps*.ips*.ups*.xdelta*.xdelta3*.vcdiff*.bps*.bsdiff*.ppf*.patch*.dps*.asm*.dldi*.xpc*.nmp*";
	TextView myPath;


	@Override

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate( savedInstanceState );
		setContentView(R.layout.filebrowser);
		File ff = new File(rootsd);
		if(ff==null||TextUtils.isEmpty(rootsd))
		{
			rootsd="/";
		}else if(!ff.exists()||!ff.canRead()||ff.isFile())
		{
			rootsd="/";
		}
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
		if(files != null)
		{
		if (files.length>0){
			
				Arrays.sort(files, new FileComparator());
			

			if(!dirPath.equals("/")&& !(dirPath.equals("/storage/") && !(new File(File.separator).canRead())))
			{
				item.add("../");
				// Thank you Marshmallow.
				// Disallowing access to /storage/emulated has now prevent billions of hacking attempts daily.
				if(new File(f.getParent()).canRead())
				{
					tmpitem=f.getParent();
					path.add(f.getParent());
				}else if (new File("/").canRead()){
					path.add("/");
					tmpitem="/";
				}else{
					path.add("/storage/");
					tmpitem = "/storage/";
				}
				
			}else{tmpitem="ROOT";}



			for(int i=0; i < files.length; i++)

			{
				File file = files[i];

				if (files[i].isFile()){
					int dotPosition = files[i].getName().lastIndexOf(".");
					String extension="";
					if (dotPosition != -1) {
						extension = (files[i].getName().substring(dotPosition)).toLowerCase(Locale.US);
						if(extensions!=null)
						{
							if(extension!=null){

								if(extensions.contains("*"+extension+"*")&&(!Globals.mode)||((Globals.mode)&&!(extensions.contains("*"+extension+"*")))){
									path.add(file.getPath());
									item.add(file.getName());
								}
							}else if(files[i].getName().endsWith("/")){
								path.add(file.getPath()+"/");
								item.add(file.getName()+"/");}else{
									path.add(file.getPath());
									item.add(file.getName());}
								}
					}else{
						path.add(file.getPath());
						item.add(file.getName());
					}
				}else{

					path.add(file.getPath()+"/");

					item.add(file.getName()+"/");}}

		}else{
			if(!dirPath.equals("/")&& !(dirPath.equals("/storage/") && !(new File(File.separator).canRead()))){
				item.add("../");
				if(new File(f.getParent()).canRead())
				{
					tmpitem=f.getParent();
					path.add(f.getParent());
				}else if (new File("/").canRead()){
					path.add("/");
					tmpitem="/";
				}else{
					path.add("/storage/");
					tmpitem = "/storage/";
				}

			}else{
				tmpitem="ROOT";
			}}
		ArrayAdapter<String> fileList =
				new ArrayAdapter<String>(this, R.layout.row, item);

		setListAdapter(fileList);
		getListView().setFastScrollEnabled(true);
		}else{
			getDir("/");
		}
	}

	@Override

	protected void onListItemClick(ListView l, View v, final int position, long id) {


		File file = new File(path.get(position));
		if (file.isDirectory())
		{
			if(file.canRead()){

				getDir(path.get(position));

			} else if (file.getAbsolutePath().equals("/storage/emulated")&&
					((new File("/storage/emulated/0").exists()&&new File("/storage/emulated/0").canRead())||
							(new File("/storage/emulated/legacy").exists()&&new File("/storage/emulated/legacy").canRead())||
							(new File("/storage/self/primary").exists()&&new File("/storage/self/primary").canRead())))
			{
				if(new File("/storage/emulated/0").exists()&&new File("/storage/emulated/0").canRead())
				{
					getDir("/storage/emulated/0");
				}else if((new File("/storage/emulated/legacy").exists()&&new File("/storage/emulated/legacy").canRead())){
					getDir("/storage/emulated/legacy");
				}else{
					getDir("/storage/self/primary");
				}
			}else
			{
				System.out.println(file.getAbsolutePath());
			
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
					if(path.get(position).toLowerCase(Locale.US).endsWith(".asm"))
					{
						AlertDialog dialog = new AlertDialog.Builder(this).create();
					    dialog.setTitle(getResources().getString(R.string.warning));
					    dialog.setMessage("ROM Patcher cannot determine if this is a Asar or Non-Asar ASM patch.\nPlease select:");
					    dialog.setCancelable(false);
					    dialog.setButton(DialogInterface.BUTTON_POSITIVE, "Asar", new DialogInterface.OnClickListener() {
					        public void onClick(DialogInterface dialog, int buttonId) {
					        	Globals.asar=true;
					        	Globals.patchToApply=path.get(position);
								 Intent returnIntent = new Intent();
								 setResult(RESULT_OK,returnIntent);
								 FileBrowserActivity.this.finish();
					        }
					    });
					    dialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Non-Asar", new DialogInterface.OnClickListener() {
					        public void onClick(DialogInterface dialog, int buttonId) {
					          Globals.asar=false;
					          Globals.patchToApply=path.get(position);
								 Intent returnIntent = new Intent();
								 setResult(RESULT_OK,returnIntent);  
								 FileBrowserActivity.this.finish();
					          
					        }
					    });
					    dialog.show();
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

}




