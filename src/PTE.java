import java.util.*;

public class PTE {

    private static boolean dirty;
    private static boolean referenced;
    private static boolean present;
    private static int ptIndex;
    private static int frameNumber;

    public PTE() {
	this.dirty = false;
	this.referenced = false;
	this.present = false;
	this.ptIndex = -1;
	this.frameNumber = -1;
    }

    // Accessor Methods
    
    public boolean isDirty(){
	return dirty;
    }

    public boolean isReferenced(){
	return referenced;
    }

    public boolean isPresent(){
	return present;
    }

    public int getIndex(){
	return ptIndex;
    }

    public int getFrameNumber(){
	return frameNumber;
    }

    // Modifier Methods

    public void setDirty(boolean sd) {
	this.dirty = sd;
    }

    public void setReferenced(boolean sr) {
	this.referenced = sr;
    }

    public void setPresent(boolean sp){
	this.present = sp;
    }

    public void setIndex(int si){
	this.ptIndex = si;
    }

    public void setFrameNumber(int sfn) {
	this.frameNumber = sfn;
    }
    

}
