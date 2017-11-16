import java.io.*;
import java.util.*;

public class vmsim {
    public static int memAccess;
    public static int pageFaults;
    public static int wtd;
    
    
    public vmsim(){
	this.memAccess = 0;
	this.pageFaults = 0;
	this.wtd = 0;
    }

    public static void main(String[] args) {
        int numFrames = 0;
        String algType = "";
        int refresh = 0;
        File traceFile = null;

        for(int i = 0; i < args.length; i++){

            switch(args[i]){
                case "-n":
                    numFrames = Integer.valueOf(args[i+1]);
                    if(numFrames < 8 || numFrames > 64 || numFrames % 8 != 0) {
                        System.err.print("Invalid frames parameter");
                        System.exit(0);
                    }
                    continue;
                case "-a":
                    algType = args[i+1];
                    continue;
                case "-r":
                    refresh = Integer.parseInt(args[i+1]);
                    continue;
                default: break;
            }

            if(i == args.length-1){
                traceFile = new File(args[i]);
            }

        }

        System.out.println("Number of frames: " + numFrames + " ,Algorithm Choice: " + algType + " ,Refesh Number: " + refresh + " :");

        if(algType.equalsIgnoreCase("opt")){
            sim_optimum(numFrames, traceFile);
        } else if(algType.equalsIgnoreCase("clock")) {
            sim_clock(numFrames, traceFile);
        } else if(algType.equalsIgnoreCase("nru")){
            sim_nru(numFrames, refresh, traceFile);
        } else if(algType.equalsIgnoreCase("random")){
            sim_random(numFrames, traceFile);
        } else {
	    System.out.println("Invalid Algorithm: Choose either Opt, Clock, NRU or Random.");
	}

	System.exit(0);

    }

    public static void sim_optimum(int nofFrames, File traceFile){}
    public static void sim_clock(int noFrames, File traceFile){}
    public static void sim_nru(int noFrames, int refresh, File traceFile){}

    public static void sim_random(int noFrames, File traceFile){
        int freeFrames = noFrames;
	    Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
	    int[] RAM = new int[noFrames];
	    for(int i = 0; i < RAM.length; i++){
	        RAM[i] = -1;
        }
	    Random rand = new Random();
	    Scanner scan;

	    try {
	        scan = new Scanner(traceFile);
	    } catch(FileNotFoundException fn) {
	        System.out.println("File not found");
	        return;
	    }


	    while(scan.hasNextLine()){
	        String trace = scan.nextLine();
            char accessType = trace.charAt(9);
            String parseAddy = "0x" + trace.substring(0, 8);
            long intAddy = Long.decode(parseAddy);
            int pageNum = (int)intAddy/(int)Math.pow(2,12);

            memAccess++;

            if(!pageTable.containsKey(pageNum)){
                PTE currentEntry = new PTE();
                pageTable.put(pageNum, currentEntry);

                if (freeFrames > 0) {
                    for (int i = 0; i < RAM.length; i++) {
                        if (RAM[i] == -1) {
                            RAM[i] = pageNum;
                            currentEntry.setFrameNumber(i);
                            currentEntry.setReferenced(true);
                            currentEntry.setPresent(true);
                            if (accessType == 'w') {
                                currentEntry.setDirty(true);
                            }
                            freeFrames--;
                        }
                    }
                } else {
                    int randomFrame = rand.nextInt(noFrames);
                }


            } else {
                PTE currentEntry = pageTable.get(pageNum);

            }



	    }
	

    }


}
