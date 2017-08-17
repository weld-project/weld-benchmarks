#!/bin/bash

echo ${TEST_HOME?"Need to set TEST_HOME"}

DATADIR=$TEST_HOME/data

rm -rf $DATADIR
mkdir $DATADIR

wget https://raw.githubusercontent.com/jvns/pandas-cookbook/master/data/311-service-requests.csv -O $DATADIR/311-service-requests-raw.csv
wget https://raw.githubusercontent.com/grammakov/USA-cities-and-states/master/us_cities_states_counties.csv -O $DATADIR/us_cities_states_counties-raw.csv

# demo and getPopulationStats data creation.
$TEST_HOME/scripts/transform-population-csv \
    -i $DATADIR/us_cities_states_counties-raw.csv \
    -o $DATADIR/us_cities_states_counties.csv

$TEST_HOME/scripts/prune-csv \
    -i $DATADIR/us_cities_states_counties.csv \
    -l "State short,Total population,Total adult population,Number of robberies" \
    -d "|"

$TEST_HOME/scripts/replicate-csv \
    -i $DATADIR/us_cities_states_counties-pruned.csv \
    -o $DATADIR/us_cities_states_counties_sf=1.csv \
    -r 1 \
    -d "|"

$TEST_HOME/scripts/replicate-csv \
    -i $DATADIR/us_cities_states_counties-pruned.csv \
    -o $DATADIR/us_cities_states_counties_sf=10.csv \
    -r 10 \
    -d "|"

$TEST_HOME/scripts/replicate-csv \
    -i $DATADIR/us_cities_states_counties-pruned.csv \
    -o $DATADIR/us_cities_states_counties_sf=100.csv \
    -r 100 \
    -d "|"

rm $DATADIR/us_cities_states_counties.csv
rm $DATADIR/us_cities_states_counties-raw.csv
rm $DATADIR/us_cities_states_counties-pruned.csv

# dataCleaning data creation.
$TEST_HOME/scripts/prune-csv \
    -i $DATADIR/311-service-requests-raw.csv \
    -l "Incident Zip" \
    -d ","

$TEST_HOME/scripts/replicate-csv \
    -i $DATADIR/311-service-requests-raw-pruned.csv \
    -o $DATADIR/311-service-requests-sf=1.csv \
    -r 1 \
    -d ","

$TEST_HOME/scripts/replicate-csv \
    -i $DATADIR/311-service-requests-raw-pruned.csv \
    -o $DATADIR/311-service-requests-sf=10.csv \
    -r 10 \
    -d ","

$TEST_HOME/scripts/replicate-csv \
    -i $DATADIR/311-service-requests-raw-pruned.csv \
    -o $DATADIR/311-service-requests-sf=100.csv \
    -r 100 \
    -d ","

rm $DATADIR/311-service-requests-raw.csv
rm $DATADIR/311-service-requests-raw-pruned.csv

