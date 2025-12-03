set terminal pngcairo enhanced size 1200,800 font 'Arial,14'
set output 'curva_robots_mangos.png'

set title 'Curva: Robots Óptimos vs Número de Mangos' font 'Arial,18'
set xlabel 'Número de Mangos' font 'Arial,14'
set ylabel 'Robots Mínimos Necesarios' font 'Arial,14'

set grid
set key left top

set datafile separator ','
set style line 1 lc rgb '#9400D3' lt 1 lw 2 pt 7 ps 1.5

plot 'curva_limpia.csv' skip 1 using 1:2 with linespoints ls 1 title 'Robots Óptimos'
