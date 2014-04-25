/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include "ScaleBar.h"

using namespace GeoVis;

double GeoVis::normalizeScaleMeters(double meters)
{
    if (meters <= 3) {
        meters = 1;
    } else if (meters <= 7.5) {
        meters = 5;
    } else if (meters <= 15) {
        meters = 10;
    } else if (meters <= 35) {
        meters = 20;
    } else if (meters <= 75) {
        meters = 50;
    } else if (meters <= 150) {
        meters = 100;
    } else if (meters <= 350) {
        meters = 200;
    } else if (meters <= 750) {
        meters = 500;
    } else if (meters <= 1500) {
        meters = 1000;
    } else if (meters <= 3500) {
        meters = 2000;
    } else if (meters <= 7500) {
        meters = 5000;
    } else if (meters <= 15000) {
        meters = 10000;
    } else if (meters <= 35000) {
        meters = 20000;
    } else if (meters <= 55000) {
        meters = 50000;
    } else if (meters <= 150000) {
        meters = 100000;
    } else if (meters <= 350000) {
        meters = 200000;
    } else if (meters <= 750000) {
        meters = 500000;
    } else if (meters <= 1500000) {
        meters = 1000000;
    } else {
        meters = 2000000;
    }
    return meters;
}

double GeoVis::normalizeScaleFeet(double feet)
{
    double feetPerMile = 5280.0;
    if (feet <= 7.5) {
        feet = 5;
    } else if (feet <= 15) {
        feet = 10;
    } else if (feet <= 35) {
        feet = 20;
    } else if (feet <= 75) {
        feet = 50;
    } else if (feet <= 150) {
        feet = 100;
    } else if (feet <= 350) {
        feet = 200;
    } else if (feet <= 750) {
        feet = 500;
    } else if (feet <= 1500) {
        feet = 1000;
    } else if (feet <= 3640) {
        feet = 2000;
    } else if (feet <= 1.5 * feetPerMile) {
        feet = 1 * feetPerMile;
    } else if (feet <= 3.5 * feetPerMile) {
        feet = 2 * feetPerMile;
    } else if (feet <= 7.5 * feetPerMile) {
        feet = 5 * feetPerMile;
    } else if (feet <= 15 * feetPerMile) {
        feet = 10 * feetPerMile;
    } else if (feet <= 35 * feetPerMile) {
        feet = 20 * feetPerMile;
    } else if (feet <= 75 * feetPerMile) {
        feet = 50 * feetPerMile;
    } else if (feet <= 150 * feetPerMile) {
        feet = 100 * feetPerMile;
    } else if (feet <= 350 * feetPerMile) {
        feet = 200 * feetPerMile;
    } else if (feet <= 750 * feetPerMile) {
        feet = 500 * feetPerMile;
    } else if (feet <= 1500 * feetPerMile) {
        feet = 1000 * feetPerMile;
    } else {
        feet = 2000 * feetPerMile;
    }
    return feet;
}

double GeoVis::normalizeScaleNauticalMiles(double nmi)
{
    //double feetPerMile = 6076.12;
    if (nmi <= 0.0015) {
        nmi = 0.001;
    } else if (nmi <= 0.0035) {
        nmi = 0.002;
    } else if (nmi <= 0.0075) {
        nmi = 0.005;
    } else if (nmi <= 0.015) {
        nmi = 0.01;
    } else if (nmi <= 0.035) {
        nmi = 0.02;
    } else if (nmi <= 0.075) {
        nmi = 0.05;
    } else if (nmi <= 0.15) {
        nmi = 0.1;
    } else if (nmi <= 0.35) {
        nmi = 0.2;
    } else if (nmi <= 0.75) {
        nmi = 0.5;
    } else if (nmi <= 1.5) {
        nmi = 1;
    } else if (nmi <= 3.5) {
        nmi = 2;
    } else if (nmi <= 7.5) {
        nmi = 5;
    } else if (nmi <= 15) {
        nmi = 10;
    } else if (nmi <= 35) {
        nmi = 20;
    } else if (nmi <= 75) {
        nmi = 50;
    } else if (nmi <= 150) {
        nmi = 100;
    } else if (nmi <= 350) {
        nmi = 200;
    } else if (nmi <= 750) {
        nmi = 500;
    } else if (nmi <= 1500) {
        nmi = 1000;
    } else {
        nmi = 2000;
    }
    return nmi;
}
