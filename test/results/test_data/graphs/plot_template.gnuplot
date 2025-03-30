#!/usr/bin/env gnuplot

# plot_template.gnuplot
# Check if variables are defined, otherwise error
title_text = ARG1
y_label = ARG2
is_log = (strlen(ARG3) > 0) ? ARG3 + 0 : 0  # Convert to number, default to 0 if not provided
input_file = ARG4
output_file = ARG5

if (!exists("title_text") || title_text eq "") {
    print "ERROR: No title provided"
    exit error 1
}
if (!exists("y_label") || y_label eq "") {
    print "ERROR: No y_label specified"
    exit error 1
}
if (!exists("input_file") || input_file eq "") {
    print "ERROR: No input file specified"
    exit error 1
}
if (!exists("output_file") || output_file eq "") {
    print "ERROR: No output file specified"
    exit error 1
}

set output output_file

# Set output format to PNG
set terminal png size 1200,800 enhanced font "Arial,12"

# Set title and labels
set title title_text font "Arial,16"
set xlabel "Number of Objects" font "Arial,14"
set ylabel y_label font "Arial,14"

# Configure grid
set grid

# Configure Y-axis scale based on is_log parameter
if (is_log) {
    set logscale y 10
    set format y "10^{%L}"
} else {
    unset logscale y
    set format y
}

# Configure legend
set key top left
set key box
set key font "Arial,12"

# Set X-axis to use the first column as labels
set datafile separator ","
set xtics rotate by 45 right
set xtics nomirror

# Skip header
set key autotitle columnhead

# Define some distinctive line colors and styles to cycle through
set style line 1 linecolor rgb "#FF0000" linewidth 1.5 pointtype 7 pointsize 1.0  # Red
set style line 2 linecolor rgb "#00FF00" linewidth 1.5 pointtype 9 pointsize 1.0  # Green
set style line 3 linecolor rgb "#0000FF" linewidth 1.5 pointtype 5 pointsize 1.0  # Blue
set style line 4 linecolor rgb "#FF00FF" linewidth 1.5 pointtype 11 pointsize 1.0 # Magenta
set style line 5 linecolor rgb "#00FFFF" linewidth 1.5 pointtype 13 pointsize 1.0 # Cyan
set style line 6 linecolor rgb "#FFFF00" linewidth 1.5 pointtype 6 pointsize 1.0  # Yellow
set style line 7 linecolor rgb "#000000" linewidth 1.5 pointtype 8 pointsize 1.0  # Black
set style line 8 linecolor rgb "#FF8000" linewidth 1.5 pointtype 4 pointsize 1.0  # Orange
set style line 9 linecolor rgb "#8000FF" linewidth 1.5 pointtype 12 pointsize 1.0 # Purple
set style line 10 linecolor rgb "#008080" linewidth 1.5 pointtype 10 pointsize 1.0 # Teal

# Safer way to get column count - creates a temporary file with the count
system("head -1 \"" . input_file . "\" | tr ',' ' ' | wc -w | tr -d '[:space:]' > column_count.tmp")
columns_str = system("cat column_count.tmp")
system("rm column_count.tmp")

# Convert to integer (ensuring we have a numeric value)
columns = int(columns_str)

# We need at least 2 columns to make a plot (X-values and at least one Y-series)
if (columns < 2) {
    print "ERROR: Not enough columns in input file (found " . columns . ", need at least 2)"
    exit error 2
}

# Build the plot command dynamically
plot_command = ""
do for [i=2:columns] {
    style_index = ((i-2) % 10) + 1  # Cycle through 10 different line styles
    if (i > 2) {
        plot_command = plot_command . ", "
    }
    plot_command = plot_command . "'" . input_file . "' using 0:" . i . ":xtic(1) with linespoints ls " . style_index
}

# Execute the dynamically built plot command
eval("plot " . plot_command)
