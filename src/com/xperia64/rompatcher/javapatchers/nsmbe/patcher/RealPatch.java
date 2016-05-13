package com.xperia64.rompatcher.javapatchers.nsmbe.patcher;

//import com.xperia64.rompatcher.javapatchers.nsmbe.AlreadyEditingException;
import com.xperia64.rompatcher.javapatchers.nsmbe.ROM;
import com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem.DSFile;

import java.io.IOException;
import java.io.RandomAccessFile;

public class RealPatch {

	private static final String patchHeader = "NSMBe4 Exported Patch";
	
	private static String readString(RandomAccessFile fs) throws IOException
	{
		byte[] tmp = new byte[fs.read()];
	    fs.read(tmp);
	    return  new String(tmp, "US-ASCII");
	}
	private static int little2bigi(int i) {
        return (i&0xff)<<24 | (i&0xff00)<<8 | (i&0xff0000)>>8 | (i>>24)&0xff;
    }
	private static short little2bigs(short s) {
        return (short) ((s&0xff)<<8 | (s&0xff00)>>8);
    }
	public static void patch(String s, Object editor) throws Exception
	{
	//output to show to the user
    boolean differentRomsWarning = false; // tells if we have shown the warning
    //int fileCount = 0;

    //open the input patch
    //if (openPatchDialog.ShowDialog() == DialogResult.Cancel)
    //    return;

    RandomAccessFile fs = new RandomAccessFile(s, "rw");
    //BinaryReader br = new BinaryReader(fs);

    byte[] tmp = new byte[fs.read()];
    fs.read(tmp);
    String header = new String(tmp, "US-ASCII");
    if (!header.equals(patchHeader))
    {
       // MessageBox.Show(
           // LanguageManager.Get("Patch", "InvalidFile"),
           // LanguageManager.Get("Patch", "Unreadable"),
           // MessageBoxButtons.OK, MessageBoxIcon.Error);
        fs.close();
        return;
    }

    //ProgressWindow progress = new ProgressWindow(LanguageManager.Get("Patch", "ImportProgressTitle"));
    //progress.Show();

    
    byte filestartByte = fs.readByte();
    /*try
    {*/
        while (filestartByte == 1)
        {
            String fileName = readString(fs);
            //progress.WriteLine(String.Format(LanguageManager.Get("Patch", "ReplacingFile"), fileName));
            short origFileID = little2bigs(fs.readShort());
            DSFile f = ROM.FS.getDSFileByName(fileName);
            long length = little2bigi(fs.readInt());
            byte[] newFile = new byte[(int) length];
            fs.read(newFile, 0, (int)length);
            filestartByte = fs.readByte();

            if (f != null)
            {
                short fileID = (short) f.id();
                
                if (!differentRomsWarning && origFileID != fileID)
                {
                    //MessageBox.Show(LanguageManager.Get("Patch", "ImportDiffVersions"), LanguageManager.Get("General", "Warning"), MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    differentRomsWarning = true;
                }
                if (!f.isSystemFile())
                {
                    f.beginEdit(editor);
                    f.replace(newFile, editor);
                    f.endEdit(editor);
                }
                //fileCount++;
            }
        }
    /*}
    catch (AlreadyEditingException e)
    {
        //MessageBox.Show(String.Format(LanguageManager.Get("Patch", "Error"), fileCount), LanguageManager.Get("General", "Completed"), MessageBoxButtons.OK, MessageBoxIcon.Error);
    	System.out.println("Error RealPatch: "+e.getCause()+" "+e.getMessage()+" "+e.getStackTrace()[0].getLineNumber()+" "+e.getStackTrace()[0].getFileName());
    }*/
    fs.close();
    //MessageBox.Show(String.Format(LanguageManager.Get("Patch", "ImportReady"), fileCount), LanguageManager.Get("General", "Completed"), MessageBoxButtons.OK, MessageBoxIcon.Information);
}
}