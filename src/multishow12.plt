#==============================================================================
# GNUPLOT-palette (dark2)
#------------------------------------------------------------------------------
# see more at https://github.com/Gnuplotting/gnuplot-palettes
#==============================================================================
# line styles for ColorBrewer Dark2
# for use with qualitative/categorical data
# provides 8 dark colors based on Set2
# compatible with gnuplot >=4.2
# author: Anna Schneider

# line styles
set style line 1 lt 1 lc rgb '#1B9E77' # dark teal
set style line 2 lt 1 lc rgb '#D95F02' # dark orange
set style line 3 lt 1 lc rgb '#7570B3' # dark lilac
set style line 4 lt 1 lc rgb '#E7298A' # dark magenta
set style line 5 lt 1 lc rgb '#66A61E' # dark lime green
set style line 6 lt 1 lc rgb '#E6AB02' # dark banana
set style line 7 lt 1 lc rgb '#A6761D' # dark tan
set style line 8 lt 1 lc rgb '#666666' # dark gray

# palette
set palette maxcolors 8
set palette defined ( 0 '#1B9E77',\
    	    	      1 '#D95F02',\
		      2 '#7570B3',\
		      3 '#E7298A',\
		      4 '#66A61E',\
		      5 '#E6AB02',\
		      6 '#A6761D',\
		      7 '#666666' )

#==============================================================================
# TERMINAL
#==============================================================================
set terminal pdf size 6.29167in,6.29167in enhanced rounded 
set encoding utf8
#==============================================================================
# OUTPUT
#==============================================================================
set output '/home/mjhmilla/Work/code/investing/eodhistoricaldata/cpp/EodHistoricalDataToolkit/data/STU/generateTickerReports/CLSD_US/fig_CLSD_US.pdf'
set encoding utf8
#==============================================================================
# MULTIPLOT
#==============================================================================
set multiplot layout 2,2 rowsfirst downwards title 'Clearside Biomedical Inc (CLSD.US) - USA'

#==============================================================================
# SETUP COMMANDS
#==============================================================================
set xrange [2016.420330:2026.607143]
set yrange [0.000000:29.664000]
set xlabel 'Year' enhanced textcolor '#404040' font 'Times,12'
set ylabel 'Adjusted Closing Price (USD)' enhanced textcolor '#404040' font 'Times,12'

set border 3 front linetype 1 linewidth 1 linecolor '#404040'
unset grid
set style fill solid noborder
set style histogram
set tics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' front
set xtics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12'
unset x2tics
set mxtics
unset mx2tics
set ytics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12'
unset y2tics
set mytics
unset my2tics
unset ztics
unset mztics
unset rtics
unset mrtics
set key inside left top opaque vertical Left noinvert reverse width 0 height 1 samplen 4 spacing 1 enhanced textcolor '#404040' font 'Times,12' nobox maxrows auto maxcols auto
set boxwidth 0.9 relative
set datafile missing "?"
#==============================================================================
# CUSTOM EXPLICIT GNUPLOT COMMANDS
#==============================================================================
set style data histogram
#==============================================================================
# PLOT COMMANDS
#==============================================================================
plot \
    'plot48.dat' index 0 title 'Clearside Biomedical Inc' with lines linestyle 1 linewidth 1 linecolor 'black', \
    'plot48.dat' index 1 notitle with lines linestyle 2 linewidth 2 linecolor 'web-blue', \
    'plot48.dat' index 2 notitle with lines linestyle 3 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot48.dat' index 3 notitle with lines linestyle 4 linewidth 2 linecolor 'web-blue', \
    'plot48.dat' index 4 notitle with lines linestyle 5 linewidth 2 linecolor 'web-blue', \
    'plot48.dat' index 5 notitle with lines linestyle 6 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot48.dat' index 6 notitle with lines linestyle 7 linewidth 2 linecolor 'web-blue', \
    'plot48.dat' index 7 notitle with lines linestyle 8 linewidth 2 linecolor 'web-blue', \
    'plot48.dat' index 8 notitle with lines linestyle 9 linewidth 1 linecolor 'web-blue', \
    'plot48.dat' index 9 notitle with lines linestyle 10 linewidth 3 linecolor 'gray100', \
    'plot48.dat' index 10 notitle with lines linestyle 11 linewidth 1 linecolor 'gray0'

#==============================================================================
# SETUP COMMANDS
#==============================================================================
set xrange [2018.243836:2022.750000]
set yrange [-1.588805:0.317761]
set xlabel 'Year' enhanced textcolor '#404040' font 'Times,12'
set ylabel 'Price To Value' enhanced textcolor '#404040' font 'Times,12'

set border 3 front linetype 1 linewidth 1 linecolor '#404040'
unset grid
set style fill solid noborder
set style histogram
set tics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' front
set xtics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12' 1
unset x2tics
set mxtics
unset mx2tics
set ytics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12'
unset y2tics
set mytics
unset my2tics
unset ztics
unset mztics
unset rtics
unset mrtics
set key inside left top opaque vertical Left noinvert reverse width 0 height 1 samplen 4 spacing 1 enhanced textcolor '#404040' font 'Times,12' nobox maxrows auto maxcols auto
set boxwidth 0.9 relative
set datafile missing "?"
#==============================================================================
# CUSTOM EXPLICIT GNUPLOT COMMANDS
#==============================================================================
set style data histogram
#==============================================================================
# PLOT COMMANDS
#==============================================================================
plot \
    'plot49.dat' index 0 title 'Clearside Biomedical Inc' with lines linestyle 1 linewidth 1 linecolor 'black', \
    'plot49.dat' index 1 notitle with lines linestyle 2 linewidth 2 linecolor 'web-blue', \
    'plot49.dat' index 2 notitle with lines linestyle 3 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot49.dat' index 3 notitle with lines linestyle 4 linewidth 2 linecolor 'web-blue', \
    'plot49.dat' index 4 notitle with lines linestyle 5 linewidth 2 linecolor 'web-blue', \
    'plot49.dat' index 5 notitle with lines linestyle 6 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot49.dat' index 6 notitle with lines linestyle 7 linewidth 2 linecolor 'web-blue', \
    'plot49.dat' index 7 notitle with lines linestyle 8 linewidth 2 linecolor 'web-blue', \
    'plot49.dat' index 8 notitle with lines linestyle 9 linewidth 1 linecolor 'web-blue', \
    'plot49.dat' index 9 notitle with lines linestyle 10 linewidth 3 linecolor 'gray100', \
    'plot49.dat' index 10 notitle with lines linestyle 11 linewidth 1 linecolor 'gray0'

#==============================================================================
# SETUP COMMANDS
#==============================================================================
set xrange [2018.243836:2026.247253]
set yrange [-186.350201:240.113383]
set xlabel 'Year' enhanced textcolor '#404040' font 'Times,12'
set ylabel 'Enterprise Value To Residual Cash Flow' enhanced textcolor '#404040' font 'Times,12'

set border 3 front linetype 1 linewidth 1 linecolor '#404040'
unset grid
set style fill solid noborder
set style histogram
set tics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' front
set xtics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12'
unset x2tics
set mxtics
unset mx2tics
set ytics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12'
unset y2tics
set mytics
unset my2tics
unset ztics
unset mztics
unset rtics
unset mrtics
set key inside left top opaque vertical Left noinvert reverse width 0 height 1 samplen 4 spacing 1 enhanced textcolor '#404040' font 'Times,12' nobox maxrows auto maxcols auto
set boxwidth 0.9 relative
set datafile missing "?"
#==============================================================================
# CUSTOM EXPLICIT GNUPLOT COMMANDS
#==============================================================================
set style data histogram
#==============================================================================
# PLOT COMMANDS
#==============================================================================
plot \
    'plot50.dat' index 0 title 'Clearside Biomedical Inc' with lines linestyle 1 linewidth 1 linecolor 'black', \
    'plot50.dat' index 1 notitle with lines linestyle 2 linewidth 2 linecolor 'web-blue', \
    'plot50.dat' index 2 notitle with lines linestyle 3 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot50.dat' index 3 notitle with lines linestyle 4 linewidth 2 linecolor 'web-blue', \
    'plot50.dat' index 4 notitle with lines linestyle 5 linewidth 2 linecolor 'web-blue', \
    'plot50.dat' index 5 notitle with lines linestyle 6 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot50.dat' index 6 notitle with lines linestyle 7 linewidth 2 linecolor 'web-blue', \
    'plot50.dat' index 7 notitle with lines linestyle 8 linewidth 2 linecolor 'web-blue', \
    'plot50.dat' index 8 notitle with lines linestyle 9 linewidth 1 linecolor 'web-blue', \
    'plot50.dat' index 9 notitle with lines linestyle 10 linewidth 3 linecolor 'gray100', \
    'plot50.dat' index 10 notitle with lines linestyle 11 linewidth 1 linecolor 'gray0'

#==============================================================================
# SETUP COMMANDS
#==============================================================================
set xrange [2018.243836:2026.247253]
set yrange [-3.642542:0.728508]
set xlabel 'Year' enhanced textcolor '#404040' font 'Times,12'
set ylabel 'Return On Invested Capital Less Cost Of Capital' enhanced textcolor '#404040' font 'Times,12'

set border 3 front linetype 1 linewidth 1 linecolor '#404040'
unset grid
set style fill solid noborder
set style histogram
set tics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' front
set xtics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12'
unset x2tics
set mxtics
unset mx2tics
set ytics border nomirror out scale 0.5,0.25 norotate enhanced textcolor '#404040' font 'Times,12'
unset y2tics
set mytics
unset my2tics
unset ztics
unset mztics
unset rtics
unset mrtics
set key inside left top opaque vertical Left noinvert reverse width 0 height 1 samplen 4 spacing 1 enhanced textcolor '#404040' font 'Times,12' nobox maxrows auto maxcols auto
set boxwidth 0.9 relative
set datafile missing "?"
#==============================================================================
# CUSTOM EXPLICIT GNUPLOT COMMANDS
#==============================================================================
set style data histogram
#==============================================================================
# PLOT COMMANDS
#==============================================================================
plot \
    'plot51.dat' index 0 title 'Clearside Biomedical Inc' with lines linestyle 1 linewidth 1 linecolor 'black', \
    'plot51.dat' index 1 notitle with lines linestyle 2 linewidth 2 linecolor 'web-blue', \
    'plot51.dat' index 2 notitle with lines linestyle 3 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot51.dat' index 3 notitle with lines linestyle 4 linewidth 2 linecolor 'web-blue', \
    'plot51.dat' index 4 notitle with lines linestyle 5 linewidth 2 linecolor 'web-blue', \
    'plot51.dat' index 5 notitle with lines linestyle 6 linewidth 1 linecolor 'web-blue' dashtype 1, \
    'plot51.dat' index 6 notitle with lines linestyle 7 linewidth 2 linecolor 'web-blue', \
    'plot51.dat' index 7 notitle with lines linestyle 8 linewidth 2 linecolor 'web-blue', \
    'plot51.dat' index 8 notitle with lines linestyle 9 linewidth 1 linecolor 'web-blue', \
    'plot51.dat' index 9 notitle with lines linestyle 10 linewidth 3 linecolor 'gray100', \
    'plot51.dat' index 10 notitle with lines linestyle 11 linewidth 1 linecolor 'gray0'

unset multiplot

set output
