# Prep

Install the [Fritzing fonts](https://fritzing.org/learning/tutorials/creating-custom-parts/download-fonts-and-templates) folder by double-clicking on them and selecting `Install Font`.

# To make parts

- create illustrator .svg files for PCB, breadboard, schematic and icon (best not to start from scratch) and save as `SVG Tiny 1.2` with advanced options `Responsive` unchecked
- create the `.fzp` file in Atom

## PCB parts

- everything needs to have the right colors, make sure in the svg for `nonconn` cutouts to include `stroke='black' stroke-width='0' fill='black'`

# To add to fritzing

 - add the pcb.svg file to `Fritzing/parts/svg/user/pcb`
 - add the breadboard.svg file to `Fritzing/parts/svg/user/pcb`
 - add the schematic.svg file to `Fritzing/parts/svg/user/schematic`
 - add the icon.svg file to `Fritzing/parts/svg/user/pcb/icon`
 - add the `.fzp` file to `Fritzing/parts/user` folder
 - add an `<instance>` block to the `my_parts.fzb` file in `Fritzing/bins` pointing to the added part(s) with the `part_id`, some unique `model_index`, and the path to the part's `.fzp` file
```
<instance moduleIdRef="part_id" modelIndex="model_index" path="path/to/part/part_id.fzp">
     <views/>
</instance>
```
