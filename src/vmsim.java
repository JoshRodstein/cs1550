/*****************************************************************************
 * cs1550 - Project 3 - Virtual Memory                                       *
 * Joshua Rodstein / jor94@pitt.edu                                          *
 *                                                                           *
 * Description: Simulate 4 Page Replacement ALgorithms                       *   
 *    Random, NRU, Clock, Optimum                                            *
 *                                                                           *
 * ./vmsim –n numframes -a <opt|clock|nru|rand> [-r refresh] tracefile       *
 *                                                                           *
 * Note: for Random, Clock, and Optimum, Refresh parameter will be ignored   * 
 *****************************************************************************/



import java.io.*;
import java.util.*;

public class vmsim {
    // variables for stats output
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

	// Flag argument parsing
        for(int i = 0; i < args.length; i++){

            switch(args[i]){
                case "-n":
                    numFrames = Integer.valueOf(args[i+1]);
                    if(numFrames < 8 || numFrames % 8 != 0) {
                        System.err.print("Invalid frames parameter");
                        System.exit(0);
                    }
                    continue;
                case "-a":
                    algType = args[i+1];
                    continue;
                case "-r":
		    try {
			refresh = Integer.parseInt(args[i+1]);
		    } catch(NumberFormatException nf){
			System.out.println("Error: invalid refresh rate");
			System.exit(0);
		    } catch(ArrayIndexOutOfBoundsException ai){
			System.out.println("Error: invalid refresh rate");
			System.exit(0);
		    }
                    continue;
                default: break;
            }

            if(i == args.length-1){
                traceFile = new File(args[i]);
            }

        }

	// Execute/verify specified algorithm
        if(algType.equalsIgnoreCase("opt")){
            sim_optimum(numFrames, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else if(algType.equalsIgnoreCase("clock")) {
            sim_clock(numFrames, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else if(algType.equalsIgnoreCase("nru")){
            sim_nru(numFrames, refresh, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else if(algType.equalsIgnoreCase("random") || algType.equalsIgnoreCase("rand")){
            sim_random(numFrames, traceFile);
            printResults(algType, numFrames, refresh, memAccess, pageFaults, wtd);
        } else {
	        System.out.println("Invalid Algorithm: Choose either Opt, Clock, NRU or Random.");
	    }

	System.exit(0);

    }

    // Simulate optimum algorithm
    public static void sim_optimum(int noFrames, File traceFile){
	/* 'clock' variable simulates clock ticks and keeps track of instruction index */
	int clock = 0;  
        int usedFrames = 0;
        Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
        PTE[] RAM = new PTE[noFrames];
        Random rand = new Random();
        Scanner scan;

        try {
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException fn) {
            System.out.println("Error: File not found");
	    System.exit(0);
            return;
        }

        while(scan.hasNextLine()){
	    // simulate rising edge by incrementing clock upon instruction read 
            clock++;
            String trace = scan.nextLine();
            String parseAddy = "0x" + trace.substring(0, 5);
            int pageNum = Integer.decode(parseAddy);

	    // Pre-Process instructions, load page table
            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
            }

	    // store instruction index in PTE contained linked list
            pageTable.get(pageNum).pushPosit(clock);
        }

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
            int pageNum = Integer.decode(parseAddy.substring(0,7));

            currentEntry = pageTable.get(pageNum);
            currentEntry.removePosit();
	    
	    memAccess++;
	    System.out.print(parseAddy + " ");
            if(currentEntry.isPresent() == false) {
		// If we are here, page not in RAM
                System.out.print("page fault -");
                pageFaults++;

		// If space, load page into 1st available frame
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') {
                        RAM[usedFrames].setDirty(true);
                    }
                    usedFrames++;
                    System.out.println(" no eviction");
                } else {
		    // if we are here, no space in RAM
                    System.out.print(" evict");
                    int evictFrame = 0;
                    int curFarthest = 0;

		    /* Iterate over position list of each PTE in every Frame
		       Compare and evict the Page with the furthest instruction */ 
                    for (int i = 0; i < RAM.length; i++) {
                        if (RAM[i].futureSize() == 0) {
                            evictFrame = i;
                            break;
                        } else {
                            if (RAM[i].peekPosit() > curFarthest) {
                                curFarthest = RAM[i].peekPosit();
                                evictFrame = i;
                            }
                        }
                    }

		    // Eviction, if Page is dirty write to disk and clean
                    RAM[evictFrame].setPresent(false);
                    RAM[evictFrame].setReferenced(false);
                    RAM[evictFrame].setFrameNumber(-1);
                    if (RAM[evictFrame].isDirty()) {
                        RAM[evictFrame].setDirty(false);
                        System.out.println(" dirty");
                        wtd++;
                    } else {
		        System.out.println(" clean");
                    }
		    // insert current page into newly freed frame
                    RAM[evictFrame] = currentEntry;
                    RAM[evictFrame].setFrameNumber(evictFrame);
                    RAM[evictFrame].setReferenced(true);
                    RAM[evictFrame].setPresent(true);
                    if (accessType == 'W') {
                        RAM[evictFrame].setDirty(true);
                    }
                }
            } else {
		// if we are here, page is in RAM and valid
                System.out.println("hit");
                if (accessType == 'W') {
                    RAM[currentEntry.getFrameNumber()].setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
            }
        }

    }
    // Simulate clock algorithm
    public static void sim_clock(int noFrames, File traceFile){
        int usedFrames = 0;
        Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
        PTE[] RAM = new PTE[noFrames];
        Scanner scan;
	// clock hand points to current oldest frame in RAM
        int clockHand = 0;

        try {
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException fn) {
            System.out.println("Error: File not found");
	    System.exit(0);
            return;
        }

        PTE currentEntry;
        while(scan.hasNextLine()){
            String trace = scan.nextLine();
            char accessType = trace.charAt(9);
	    String parseAddy = "0x" + trace.substring(0, 8);
            int pageNum = Integer.decode(parseAddy.substring(0,7));

	    // Dyamically Load PageTable
            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
                currentEntry = newEntry;
            } else {
                currentEntry = pageTable.get(pageNum);
            }

            memAccess++;
	    System.out.print(parseAddy + " ");
            if(currentEntry.isPresent() == false) {
		// if we are, page is not in RAM
                pageFaults++;
                System.out.print("page fault -");
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') { RAM[usedFrames].setDirty(true); }
                    usedFrames++;
                    System.out.println(" no eviction");
                } else {
		    // if we are here, no available Frame in RAM
		    System.out.print(" evict");

		    /* Search for oldest NOT-referenced page by examining
		       frame pointed to by clock hand. Simulates circular
		       queue. */
                    int evictFrame;
                    while(true){
                        if(clockHand == noFrames){
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

		    // evict frame found in previous iteration
                    RAM[evictFrame].setPresent(false);
                    RAM[evictFrame].setReferenced(false);
                    RAM[evictFrame].setFrameNumber(-1);

                    if (RAM[evictFrame].isDirty()) {
                        RAM[evictFrame].setDirty(false);
                        System.out.println(" dirty");
                        wtd++;
                    } else {
			System.out.println(" clean");
                    }

		    // place current page into newly freed frame
                    RAM[evictFrame] = currentEntry;
                    RAM[evictFrame].setFrameNumber(evictFrame);
                    RAM[evictFrame].setReferenced(true);
                    RAM[evictFrame].setPresent(true);
                    if (accessType == 'W') { RAM[evictFrame].setDirty(true); }
                }
            } else {
		// If we are here, page is valid / exists in RAM
		System.out.println("hit");
                if (accessType == 'W') {
                    RAM[currentEntry.getFrameNumber()].setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
            }
        }
    }
    // simulate not recently used algorithm
    public static void sim_nru(int noFrames, int refresh, File traceFile){
        int usedFrames = 0;
        Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
        PTE[] RAM = new PTE[noFrames];
        Random rand = new Random();
        Scanner scan;

        try {
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException fn) {
            System.out.println("Error: File not found");
	    System.exit(0);
            return;
        }

        PTE currentEntry;
	// Array of LinkedLists holds 4 classes of referenced/dirty bit permuations
        LinkedList<Integer>[] nruClass = new LinkedList[4];

        while(scan.hasNextLine()){
            String trace = scan.nextLine();
            char accessType = trace.charAt(9);
            String parseAddy = "0x" + trace.substring(0, 8);
            int pageNum = Integer.decode(parseAddy.substring(0,7));

            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
                currentEntry = newEntry;
            } else {
                currentEntry = pageTable.get(pageNum);
            }

            memAccess++;
	    System.out.print(parseAddy + " ");
            if(currentEntry.isPresent() == false) {
                System.out.print("page fault -");
                pageFaults++;
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') { RAM[usedFrames].setDirty(true); }
                    usedFrames++;
                    System.out.println(" no eviction");
                } else {
                    System.out.print(" evict");
		    //lnec: holds lowest none empty class index
                    int lnec = -1;

                    for(int i = 0; i < 4; i++){
                        nruClass[i] = new LinkedList<Integer>();
                    }
		    /* If eviction is needed, iterate over Frames in RAM
		     organizing by perm of referenced and dirty bit values
		     placing index into corresponding nruClass LinkedList */
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

		    // store lowest none empty class index
                    for(int i = 0; i < nruClass.length; i++) {
                        if (nruClass[i].size() > 0) {
                            lnec = i;
                            break;
                        }
                    }

		    // pick a random index from lowest none empty class to evict
                    int randomFrame = rand.nextInt(nruClass[lnec].size());
                    int evictFrame = nruClass[lnec].get(randomFrame);
                    pageTable.get(RAM[evictFrame].getIndex()).setPresent(false);
                    pageTable.get(RAM[evictFrame].getIndex()).setReferenced(false);
                    pageTable.get(RAM[evictFrame].getIndex()).setFrameNumber(-1);

                    if (pageTable.get(RAM[evictFrame].getIndex()).isDirty()) {
                        pageTable.get(RAM[evictFrame].getIndex()).setDirty(false);
                        System.out.println(" dirty");
                        wtd++;
                    } else {
			System.out.println(" clean");
                    }

		    // place current page into newly freed frame
                    RAM[evictFrame] = currentEntry;
                    currentEntry.setFrameNumber(evictFrame);
                    currentEntry.setReferenced(true);
                    currentEntry.setPresent(true);
                    if (accessType == 'W') { currentEntry.setDirty(true); }

                }
            } else {
                System.out.println("hit");
                if (accessType == 'W') {
                    currentEntry.setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
            }

	    if(refresh == 0) {
		continue;
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
    // simulate random algorithm
    public static void sim_random(int noFrames, File traceFile){
        int usedFrames = 0;
	    Hashtable<Integer, PTE> pageTable = new Hashtable<Integer, PTE>();
	    PTE[] RAM = new PTE[noFrames];
	    Random rand = new Random();
	    Scanner scan;

	    try {
	        scan = new Scanner(traceFile);
	    } catch(FileNotFoundException fn) {
	        System.out.println("Error: File not found");
		System.exit(0);
	        return;
	    }

        PTE currentEntry;
	    while(scan.hasNextLine()){
            String trace = scan.nextLine();
            char accessType = trace.charAt(9);
            String parseAddy = "0x" + trace.substring(0, 8);
            int pageNum = Integer.decode(parseAddy.substring(0,7));

            if (pageTable.containsKey(pageNum) == false) {
                PTE newEntry = new PTE();
                newEntry.setIndex(pageNum);
                pageTable.put(pageNum, newEntry);
                currentEntry = newEntry;
            } else {
                currentEntry = pageTable.get(pageNum);
            }

            memAccess++;
            System.out.print(parseAddy + " ");
            if(currentEntry.isPresent() == false) {
                System.out.print("page fault -");
                pageFaults++;
                if (usedFrames < noFrames) {
                    RAM[usedFrames] = currentEntry;
                    RAM[usedFrames].setFrameNumber(usedFrames);
                    RAM[usedFrames].setReferenced(true);
                    RAM[usedFrames].setPresent(true);
                    if (accessType == 'W') { RAM[usedFrames].setDirty(true); }
                    usedFrames++;
                    System.out.println(" no eviction");
                } else {
		    System.out.print(" evict");
		    // evict random frame from RAM
                    int randomFrame = rand.nextInt(noFrames);
                    int evictIndex = RAM[randomFrame].getIndex();
                    pageTable.get(evictIndex).setPresent(false);
                    pageTable.get(evictIndex).setReferenced(false);
                    pageTable.get(evictIndex).setFrameNumber(-1);

                    if (pageTable.get(evictIndex).isDirty()) {
                        pageTable.get(evictIndex).setDirty(false);
                        System.out.println(" dirty");
                        wtd++;
                    } else {
			System.out.println(" clean");
                    }

                    RAM[randomFrame] = currentEntry;
                    RAM[randomFrame].setFrameNumber(randomFrame);
                    RAM[randomFrame].setReferenced(true);
                    RAM[randomFrame].setPresent(true);
                    if (accessType == 'W') {
                        RAM[randomFrame].setDirty(true);
                    }

                }
            } else {
                if (accessType == 'W') {
                    RAM[currentEntry.getFrameNumber()].setDirty(true);
                }
                RAM[currentEntry.getFrameNumber()].setReferenced(true);
                System.out.println("hit");
            }
        }
    }
    
    // print end of program statistics
    public static void printResults(String algType, int numFrames, int refresh,
                                    int memAccess, int pageFaults, int wtd){
	System.out.println();
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
