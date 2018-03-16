# set terminal pngcairo  transparent enhanced font "arial,10" fontscale 1.0 size 600, 400 
# set output 'heatmaps.2.png'
set terminal x11 persist 
unset key
set view map 
set style data lines
set xtics border in scale 0,0 mirror norotate  autojustify
set ytics border in scale 0,0 mirror norotate  autojustify
set ztics border in scale 0,0 nomirror norotate  autojustify
unset cbtics
set rtics axis in scale 0,0 nomirror norotate  autojustify
#set xrange [ 0 :  ] noreverse nowriteback
#set yrange [ 0 : 2 ] noreverse nowriteback
set cblabel "Scale" font "Times-Roman, 20"
set xlabel "Samples" font "Times-Roman, 20"
set ylabel "Localities" font "Times-Roman, 20" 
set cbrange [ 0.00000 : 100.00000 ] noreverse nowriteback
#set palette rgbformulae -7, 2, -7
set zrange [ 0.0000 : 100.0000 ]
set palette defined (0 "white", 100 "red")
## Last datafile plotted: "$map2"
plot filename using 2:1:3 with image
