#OPTS = -O2 -DOPTIMIZE -DUSE_CMIPOOL -DFIRST_SOLUTION -DUSE_PRIORITIES #-g #-DBOUNDCHECK #-DSE_DEBUG #-DMEMCHECK 
#OPTS = -O2 -DOPTIMIZE -DUSE_CMIPOOL -DUSE_PRIORITIES #-g #-DBOUNDCHECK #-DSE_DEBUG #-DMEMCHECK 
OPTS = -O2 -g

# Fill in the path to CHARMC here
#CHARMC_PATH=~/charmgit/charm/bin
#CHARMC_PATH=~/work/CHARM/charm/bin
CHARMC_PATH=/home/mdrichar/software/charm-6.4.0/bin
CHARMC=$(CHARMC_PATH)/charmc $(OPTS)

####################################
# Modify targets below this line to
# build.
####################################

#all: hc hc.rndm hc.wkstl hc.nbr  
all: hc 

hc: hc.o
	$(CHARMC) -language charm++  hc.o -o hc -module searchEngine  -memory paranoid

#hc: searchEngine.o hc.o
#	$(CHARMC) -language charm++  hc.o searchEngine.o -o hc -tracemode projections #-memory paranoid

#hc.rndm: searchEngine.o hc.o
#	$(CHARMC) -language charm++  hc.o searchEngine.o -o hc.rndm -balance rand #-memory paranoid

#hc.wkstl: searchEngine.o hc.o
#	$(CHARMC) -language charm++  hc.o searchEngine.o -o hc.wkstl -balance workstealing #-memory paranoid

#hc.nbr: searchEngine.o hc.o
#	$(CHARMC) -language charm++  hc.o searchEngine.o -o hc.nbr -balance neighbor #-memory paranoid

#hc_prj: searchEngine.o hc.o
#	$(CHARMC) -language charm++  -tracemode projections hc.o searchEngine.o -o hc_prj

hc.decl.h: hc.ci
	$(CHARMC)  hc.ci

hc.o: hc.C gameLogic.h hc.decl.h
	$(CHARMC) -c  hc.C

####################################
# You don't need to modify anything 
# below this line.
####################################

#searchEngine.o: searchEngineAPI.h searchEngine.h searchEngine.C searchEngine.decl.h
#	$(CHARMC) -c  searchEngine.C

#searchEngine.decl.h: searchEngine.ci
#	$(CHARMC) -E  searchEngine.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o hc hc.rndm hc.wkstl hc.nbr  charmrun *~
