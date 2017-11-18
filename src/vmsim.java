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
        int refresh = -1;
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

        System.out.println("Number of frames: " + numFrames + " ,Algorithm Choice: " + algType +
                " ,Refesh Number: " + refresh + " \n");

        if(algType.equalsIgnoreCase("opt")){
            sim_optimum(numFrames, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else if(algType.equalsIgnoreCase("clock")) {
            sim_clock(numFrames, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else if(algType.equalsIgnoreCase("nru")){
            sim_nru(numFrames, refresh, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else if(algType.equalsIgnoreCase("random")){
            sim_random(numFrames, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else {
	        System.out.println("Invalid Algorithm: Choose either Opt, Clock, NRU or Random.");
	    }

	System.exit(0);

    }

    public static void sim_optimum(int noFrames, File traceFile){
        int clock = 0;
        int usedFrames = 0;
        Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
        PTE[] RAM = new PTE[noFrames];
        Random rand = new Random();
        Scanner scan;

        try {
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException fn) {
            System.out.println("File not found");
            return;
        }

        while(scan.hasNextLine()){
            clock++;
            String trace = scan.nextLine();
            String parseAddy = "0x" + trace.substring(0, 8);
            long intAddy = Long.decode(parseAddy);
            int pageNum = (int)intAddy/(int)Math.pow(2,12);

            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
            }

            pageTable.get(pageNum).pushPosit(clock);
        }

        System.out.println("Clock Cycles: " + clock);
        //System.exit(0);

        try {
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException fn) {
            System.out.println("File not found");
            return;
        }

        PTE currentEntry;
        while(scan.hasNextLine()){
            String trace = scan.nextLine();
            char accessType = trace.charAt(9);
            String parseAddy = "0x" + trace.substring(0, 8);
            long intAddy = Long.decode(parseAddy);
            int pageNum = (int)intAddy/(int)Math.pow(2,12);

            currentEntry = pageTable.get(pageNum);

            memAccess++;
            if(currentEntry.isPresent() == false) {
                pageFaults++;
                currentEntry.removePosit();
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') { RAM[usedFrames].setDirty(true); }
                    usedFrames++;
                } else {

                    int evictFrame = 0;
                    int curFarthest = 0;

                    for(int i = 0; i < RAM.length; i++){
                        if(RAM[i].futureSize() == 0) {
                            evictFrame = i;
                            break;
                        } else {
                            if(RAM[i].peekPosit() > curFarthest){
                                curFarthest = RAM[i].peekPosit();
                                evictFrame = i;
                            }
                        }
                    }

                    int evictIndex = RAM[evictFrame].getIndex();
                    pageTable.get(evictIndex).setPresent(false);
                    pageTable.get(evictIndex).setReferenced(false);
                    pageTable.get(evictIndex).setFrameNumber(-1);

                    if (pageTable.get(evictIndex).isDirty()) {
                        pageTable.get(evictIndex).setDirty(false);
                        wtd++;
                    }

                    RAM[evictFrame] = currentEntry;
                    RAM[evictFrame].setFrameNumber(evictFrame);
                    RAM[evictFrame].setReferenced(true);
                    RAM[evictFrame].setPresent(true);
                    if (accessType == 'W') { RAM[evictFrame].setDirty(true); }
                }
            } else {
                if (accessType == 'W') {
                    RAM[currentEntry.getFrameNumber()].setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
                RAM[currentEntry.getFrameNumber()].removePosit();
            }
        }

    }
    public static void sim_clock(int noFrames, File traceFile){
        int usedFrames = 0;
        Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
        PTE[] RAM = new PTE[noFrames];
        Random rand = new Random();
        Scanner scan;
        int clockHand = 0;

        try {
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException fn) {
            System.out.println("File not found");
            return;
        }

        PTE currentEntry;
        while(scan.hasNextLine()){
            String trace = scan.nextLine();
            char accessType = trace.charAt(9);
            String parseAddy = "0x" + trace.substring(0,8);
            long intAddy = Long.decode(parseAddy);
            int pageNum = (int)intAddy/(int)Math.pow(2,12);

            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
                currentEntry = newEntry;
            } else {
                currentEntry = pageTable.get(pageNum);
            }

            memAccess++;
            if(currentEntry.isPresent() == false) {
                pageFaults++;
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') { RAM[usedFrames].setDirty(true); }
                    usedFrames++;
                } else {
                    int evictFrame;
                    while(true){
                        if(clockHand == 8){
                            clockHand = 0;
                        }
                        if(RAM[clockHand].isReferenced() == false){
                            evictFrame = clockHand;
                            clockHand++;
                            break;
                        } else {
                            RAM[clockHand].setReferenced(false);
                            clockHand++;
                        }
                    }

                    int evictIndex = RAM[evictFrame].getIndex();
                    pageTable.get(evictIndex).setPresent(false);
                    pageTable.get(RAM[evictFrame].getIndex()).setReferenced(false);
                    pageTable.get(evictIndex).setFrameNumber(-1);

                    if (pageTable.get(evictIndex).isDirty()) {
                        pageTable.get(evictIndex).setDirty(false);
                        wtd++;
                    }

                    RAM[evictFrame] = currentEntry;
                    RAM[evictFrame].setFrameNumber(evictFrame);
                    RAM[evictFrame].setReferenced(true);
                    RAM[evictFrame].setPresent(true);
                    if (accessType == 'W') { RAM[evictFrame].setDirty(true); }
                }
            } else {
                if (accessType == 'W') {
                    RAM[currentEntry.getFrameNumber()].setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
            }
        }
    }
    public static void sim_nru(int noFrames, int refresh, File traceFile){
        int usedFrames = 0;
        Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
        PTE[] RAM = new PTE[noFrames];
        Random rand = new Random();
        Scanner scan;

        try {
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException fn) {
            System.out.println("File not found");
            return;
        }

        PTE currentEntry;
        LinkedList<Integer>[] nruClass = new LinkedList[4];

        while(scan.hasNextLine()){
            String trace = scan.nextLine();
            char accessType = trace.charAt(9);
            String parseAddy = "0x" + trace.substring(0, 8);
            long intAddy = Long.decode(parseAddy);
            int pageNum = (int)intAddy/(int)Math.pow(2,12);



            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
                currentEntry = newEntry;
            } else {
                currentEntry = pageTable.get(pageNum);
            }

            memAccess++;
            if(currentEntry.isPresent() == false) {
                pageFaults++;
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') { RAM[usedFrames].setDirty(true); }
                    usedFrames++;
                } else {
                    int lnec = -1;

                    for(int i = 0; i < 4; i++){
                        nruClass[i] = new LinkedList<Integer>();
                    }

                    for(int i = 0; i < RAM.length; i++){
                        if(RAM[i].isReferenced() == false && RAM[i].isDirty() == false){
                            nruClass[0].push(i);
                        } else if (RAM[i].isReferenced() == false && RAM[i].isDirty() == true){
                            nruClass[1].push(i);
                        } else if (RAM[i].isReferenced() == true && RAM[i].isDirty() == false){
                            nruClass[2].push(i);
                        } else if (RAM[i].isReferenced() == true && RAM[i].isDirty() == true){
                            nruClass[3].push(i);
                        }
                    }

                    for(int i = 0; i < nruClass.length; i++) {
                        if (nruClass[i].size() > 0) {
                            lnec = i;
                            break;
                        }
                    }


                    int randomFrame = rand.nextInt(nruClass[lnec].size());
                    int evictFrame = nruClass[lnec].get(randomFrame);
                    pageTable.get(RAM[evictFrame].getIndex()).setPresent(false);
                    pageTable.get(RAM[evictFrame].getIndex()).setReferenced(false);
                    pageTable.get(RAM[evictFrame].getIndex()).setFrameNumber(-1);

                    if (pageTable.get(RAM[evictFrame].getIndex()).isDirty()) {
                        pageTable.get(RAM[evictFrame].getIndex()).setDirty(false);
                        wtd++;
                    }

                    RAM[evictFrame] = currentEntry;
                    currentEntry.setFrameNumber(evictFrame);
                    currentEntry.setReferenced(true);
                    currentEntry.setPresent(true);
                    if (accessType == 'W') { currentEntry.setDirty(true); }

                }
            } else {
                if (accessType == 'W') {
                    currentEntry.setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
            }


            if(memAccess % refresh == 0){
                for(int i = 0; i < usedFrames; i++){
                    if(RAM[i].isReferenced()) {
                        RAM[i].setReferenced(false);
                    }
                }

            }
        }
    }
    public static void sim_random(int noFrames, File traceFile){
        int usedFrames = 0;
	    Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
	    PTE[] RAM = new PTE[noFrames];
	    Random rand = new Random();
	    Scanner scan;

	    try {
	        scan = new Scanner(traceFile);
	    } catch(FileNotFoundException fn) {
	        System.out.println("File not found");
	        return;
	    }

        PTE currentEntry;
	    while(scan.hasNextLine()){
	        String trace = scan.nextLine();
            char accessType = trace.charAt(9);
            String parseAddy = "0x" + trace.substring(0, 8);
            long intAddy = Long.decode(parseAddy);
            int pageNum = (int)intAddy/(int)Math.pow(2,12);

            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
                currentEntry = newEntry;
            } else {
                currentEntry = pageTable.get(pageNum);
            }

            memAccess++;
            if(currentEntry.isPresent() == false) {
                pageFaults++;
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') { RAM[usedFrames].setDirty(true); }
                    usedFrames++;
                } else {
                    int randomFrame = rand.nextInt(noFrames);
                    int evictIndex = RAM[randomFrame].getIndex();
                    pageTable.get(evictIndex).setPresent(false);
                    pageTable.get(evictIndex).setReferenced(false);
                    pageTable.get(evictIndex).setFrameNumber(-1);

                    if (pageTable.get(evictIndex).isDirty()) {
                        pageTable.get(evictIndex).setDirty(false);
                        wtd++;
                    }

                    RAM[randomFrame] = currentEntry;
                    RAM[randomFrame].setFrameNumber(randomFrame);
                    RAM[randomFrame].setReferenced(true);
                    RAM[randomFrame].setPresent(true);
                    if (accessType == 'W') { RAM[randomFrame].setDirty(true); }
                }
            } else {
                if (accessType == 'W') {
                    RAM[currentEntry.getFrameNumber()].setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
            }
        }
    }

    public static void printResults(String algType, int numFrames, int refresh,
                                    int memAccess, int pageFaults, int wtd){
        System.out.println("Algorithm: " + algType);
        System.out.println("Number of frames: " + numFrames);
        if(refresh >= 0 && algType.equalsIgnoreCase("nru")){
            System.out.println("Refresh rate: " + refresh);
        }
        System.out.println("Total memory accesses: " + memAccess);
        System.out.println("Total memory page faults: " + pageFaults);
        System.out.println("Total writes to disk: " + wtd);
    }


}
