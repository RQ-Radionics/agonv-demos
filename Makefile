# Makefile raíz - compila demos de agonV para ESP32-MOS
#
# Uso:
#   make                      → compila todas las demos (default: esp32p4)
#   make TARGET=esp32s3       → compila para ESP32-S3 (Xtensa)
#   make mandelbrot           → compila solo mandelbrot.bin
#   make install              → copia .bin a ../agon-lite-v/data/<TARGET>/demos/
#   make clean

TARGET ?= esp32p4

DEMOS     = mandelbrot 3dcube hatgraph
DATA_DIR  = $(abspath ../agon-lite-v/data/$(TARGET)/demos)

.PHONY: all clean install $(DEMOS)

all: $(DEMOS)

$(DEMOS):
	$(MAKE) -C $@ TARGET=$(TARGET)

install: all
	@mkdir -p $(DATA_DIR)
	@for demo in $(DEMOS); do \
	    bin=$$demo/bin/*.bin; \
	    for b in $$bin; do \
	        [ -f "$$b" ] || continue; \
	        name=$$(basename $$b); \
	        cp "$$b" $(DATA_DIR)/$$name; \
	        echo "  → $(DATA_DIR)/$$name"; \
	    done; \
	done

clean:
	for demo in $(DEMOS); do $(MAKE) -C $$demo clean; done
