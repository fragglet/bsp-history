# $Id: Makefile.am,v 1.6 2001/11/17 15:42:43 cph Exp $

BSP = ../bsp
WADS = bsp23bug.wad conduits.wad transdor.wad visarea.wad
UNWADS = bsp23bug.unbuiltwad conduits.unbuiltwad transdor.unbuiltwad visarea.unbuiltwad
MD5 = md5sum
DOOM_WAD=/usr/local/share/games/doom/doom.wad
TIME=time

EXTRA_DIST = conduits.unbuiltwad transdor.unbuiltwad visarea.unbuiltwad MD5SUMS

%.wad	:	%.unbuiltwad
	$(TIME) $(BSP) -o $@ $<

doom.wad	:	$(DOOM_WAD)
	$(TIME) $(BSP) -o $@ $(DOOM_WAD)

test	:	$(WADS) doom.wad
	$(MD5) $(WADS) doom.wad | diff MD5SUMS -

clean	:
	rm -f $(WADS) doom.wad

