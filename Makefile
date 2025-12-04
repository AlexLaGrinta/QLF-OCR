.PHONY: all clean

all:
	$(MAKE) -C Interface
	$(MAKE) -C Preprocessing
	$(MAKE) -C RotationPNG
	$(MAKE) -C segmentation
	$(MAKE) -C solver-cli
	$(MAKE) -C XNORNetwork

clean:
	$(MAKE) -C Interface clean
	$(MAKE) -C Preprocessing clean
	$(MAKE) -C RotationPNG clean
	$(MAKE) -C segmentation clean
	$(MAKE) -C solver-cli clean
	$(MAKE) -C XNORNetwork clean


