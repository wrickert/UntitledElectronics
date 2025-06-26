#!/bin/bash

[ -d "panel" ] || mkdir -p "panel"
kikit panelize \
   --layout 'grid; rows: 5; cols: 5; space: 2mm' \
   --tabs 'annotation' \
   --cuts 'mousebites; drill: 0.5mm; spacing: 1mm; offset: 0.2mm; prolong: 0.5mm' \
   --framing 'frame; width: 5mm; space: 3mm; cuts: h' \
   --tooling '4hole; hoffset: 2.5mm; voffset: 2.5mm; size: 1.5mm' \
   --fiducials '3fid; hoffset: 5mm; voffset: 2.5mm; coppersize: 2mm; opening: 1mm;' \
   --text 'simple; text: jlcjlcjlcjlc; anchor: mt; voffset: 2.5mm; hjustify: center; vjustify: center;' \
   --post 'millradius: 1mm' \
   $1 panel/panel.kicad_pcb
