import java.util.ArrayList;
import java.util.LinkedList;

public class PTE {

    private boolean dirty;
    private boolean referenced;
    private boolean present;
    private int ptIndex;
    private int frameNumber;
    private ArrayList<Integer> futures;

    public PTE() {
        this.dirty = false;
        this.referenced = false;
        this.present = false;
        this.ptIndex = -1;
        this.frameNumber = -1;
        futures = new ArrayList<Integer>();
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

    public int removePosit(){ return (futures.remove(0)); }

    public int peekPosit(){ return (futures.get(0)); }

    public int futureSize(){
        return futures.size();
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

    public boolean pushPosit(int instructionIndex){
        return (futures.add(instructionIndex));
    }





    

}
