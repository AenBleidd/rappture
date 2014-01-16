/* 
 * ----------------------------------------------------------------------
 *  RpDicomToVtk - 
 *
 * ======================================================================
 *  AUTHOR:  Leif Delgass, Purdue University
 *  Copyright (c) 2014  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkIntArray.h>
#include <vtkDataSetWriter.h>
#include <vtkDirectory.h>
#ifdef USE_VTK_DICOM_PACKAGE
#include <vtkDICOMSorter.h>
#include <vtkDICOMReader.h>
#include <vtkDICOMMetaData.h>
#else
#include <vtkDICOMImageReader.h>
#endif

#include <stdio.h>
#include "tcl.h"

static int
DicomToVtkCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                         Tcl_GetString(objv[0]), " dir|files fileList\"", (char*)NULL);
        return TCL_ERROR;
    }
    bool isDir = false;
    char *arg = Tcl_GetString(objv[1]);
    if (arg[0] == 'd' && strcmp(arg, "dir") == 0) {
        isDir = true;
    }

    char *dirName = NULL;
    vtkSmartPointer<vtkStringArray> pathArray = vtkSmartPointer<vtkStringArray>::New();
    if (isDir) {
        dirName = Tcl_GetString(objv[2]); 
    } else {
        // Get path array from Tcl list
        int pathListc;
        Tcl_Obj **pathListv = NULL;
        if (Tcl_ListObjGetElements(interp, objv[2], &pathListc, &pathListv) != TCL_OK) {
            return TCL_ERROR;
        }
        pathArray->SetNumberOfValues(pathListc);
        for (int i = 0; i < pathListc; i++) {
            pathArray->SetValue(i, Tcl_GetString(pathListv[i]));
        }
    }

    vtkSmartPointer<vtkDICOMReader> reader = vtkSmartPointer<vtkDICOMReader>::New();
#ifdef USE_VTK_DICOM_PACKAGE
    vtkSmartPointer<vtkDICOMSorter> sorter = vtkSmartPointer<vtkDICOMSorter>::New();
    if (isDir) {
        vtkSmartPointer<vtkDirectory> dir = vtkSmartPointer<vtkDirectory>::New();
        if (!dir->Open(dirName)) {
            return TCL_ERROR;
        }

        int numFiles = dir->GetNumberOfFiles();
        for (int i = 0; i < numFiles; i++) {
            if (dir->GetFile(i)[0] == '.') {
                continue;
            }

            std::string path(dirName);
            path += "/";
            path += dir->GetFile(i);

            if (!dir->FileIsDirectory(path.c_str()) &&
                reader->CanReadFile(path.c_str())) {
                pathArray->InsertNextValue(path);
            } else {
                fprintf(stderr, "Skipping file %s\n", path.c_str());
            }
        }
    }

    sorter->SetInputFileNames(pathArray);
    sorter->Update();

    int numStudies = sorter->GetNumberOfStudies();
    int study = 0;
    int series = 0;

    fprintf(stderr, "Num Studies: %d\n", numStudies);
    vtkStringArray *files;
#if 0
    for (int i = 0; i < numStudies; i++) {
        int numSeries = sorter->GetNumberOfSeriesInStudy(i);
        fprintf(stderr, "Study %d: %d series\n", i, numSeries);
        int k = sorter->GetFirstSeriesInStudy(i);
        for (int j = 0; j < numSeries; j++) {
            files = sorter->GetFileNamesForSeries(k+j);
        }
    }
#else
    if (study < 0 || study > sorter->GetNumberOfStudies()-1) {
        return TCL_ERROR;
    }
    if (series < 0 || series > sorter->GetNumberOfSeriesInStudy(study)-1) {
        return TCL_ERROR;
    }
    files = sorter->GetFileNamesForSeries(sorter->GetFirstSeriesInStudy(study)+series);
#endif

    Tcl_Obj *objPtr = Tcl_NewListObj(0, NULL);
    Tcl_Obj *metaDataObj = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("num_studies", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(numStudies));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("study", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(study));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("num_series", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(sorter->GetNumberOfSeriesInStudy(study)));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("series", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(series));

    reader->SetFileNames(files);
#else
    Tcl_Obj *objPtr = Tcl_NewListObj(0, NULL);
    Tcl_Obj *metaDataObj = Tcl_NewListObj(0, NULL);

    vtkSmartPointer<vtkDICOMImageReader> reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    if (isDir) {
        reader->SetDirectoryName(dirName);
    } else {
        reader->SetFileName(pathArray->GetValue(0));
    }
#endif
    reader->Update();

    Tcl_Obj *eltObj;
#ifdef USE_VTK_DICOM_PACKAGE
    vtkStringArray *ids = reader->GetStackIDs();
    for (int i = 0; i < ids->GetNumberOfValues(); i++) {
        fprintf(stderr, "Stack: %s\n", ids->GetValue(i).c_str());
    }
    vtkIntArray *fidxArray = reader->GetFileIndexArray();
    vtkDICOMMetaData *md = reader->GetMetaData();
    int numComp = fidxArray->GetNumberOfComponents();
#if 0
    for (int i = 0; i < fidxArray->GetNumberOfTuples(); i++) {
        fprintf(stderr, "%d:", i);
        for (int j = 0; j < fidxArray->GetNumberOfComponents(); j++) {
            int idx = (int)fidxArray->GetComponent(i, j);
            fprintf(stderr, " %d", idx);
        }
        fprintf(stderr, "\n");
    }
#endif
    fprintf(stderr, "Number of data elements: %d\n", md->GetNumberOfDataElements());

    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("num_components", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(numComp));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("modality", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(md->GetAttributeValue(DC::Modality).AsString().c_str(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("patient_name", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(md->GetAttributeValue(DC::PatientName).AsString().c_str(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("transfer_syntax_uid", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(md->GetAttributeValue(DC::TransferSyntaxUID).AsString().c_str(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("study_uid", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(md->GetAttributeValue(DC::StudyInstanceUID).AsString().c_str(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("study_id", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(md->GetAttributeValue(DC::StudyID).AsString().c_str(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("series_number", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(md->GetAttributeValue(DC::SeriesNumber).AsInt()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("series_in_study", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(md->GetAttributeValue(DC::SeriesInStudy).AsInt()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("series_uid", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(md->GetAttributeValue(DC::SeriesInstanceUID).AsString().c_str(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("bits_allocated", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(md->GetAttributeValue(DC::BitsAllocated).AsInt()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("bits_stored", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(md->GetAttributeValue(DC::BitsStored).AsInt()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("pixel_representation", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(md->GetAttributeValue(DC::PixelRepresentation).AsString().c_str(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("rescale_slope", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewDoubleObj(md->GetAttributeValue(DC::RescaleSlope).AsDouble()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("rescale_intercept", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewDoubleObj(md->GetAttributeValue(DC::RescaleIntercept).AsDouble()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("time_dimension", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(reader->GetTimeDimension()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("time_spacing", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewDoubleObj(reader->GetTimeSpacing()));

    fprintf(stderr, "Time dim: %d, spacing: %g\n", reader->GetTimeDimension(), reader->GetTimeSpacing());
    fprintf(stderr, "Number of files: %d\n", md->GetNumberOfInstances());
#else
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("patient_name", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(reader->GetPatientName(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("transfer_syntax_uid", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(reader->GetTransferSyntaxUID(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("study_uid", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(reader->GetStudyUID(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("study_id", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj(reader->GetStudyID(), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("num_components", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(reader->GetNumberOfComponents()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("bits_allocated", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewIntObj(reader->GetBitsAllocated()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("pixel_representation", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj,
                             Tcl_NewStringObj((reader->GetPixelRepresentation() ? "signed" : "unsigned"), -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("rescale_slope", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewDoubleObj(reader->GetRescaleSlope()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("rescale_offset", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewDoubleObj(reader->GetRescaleOffset()));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewStringObj("gantry_angle", -1));
    Tcl_ListObjAppendElement(interp, metaDataObj, Tcl_NewDoubleObj(reader->GetGantryAngle()));

    fprintf(stderr, "Patient name: %s\n", reader->GetPatientName());
    fprintf(stderr, "Transfer Syntax UID: %s\n", reader->GetTransferSyntaxUID());
    fprintf(stderr, "Study UID: %s\n", reader->GetStudyUID());
    fprintf(stderr, "Study ID: %s\n", reader->GetStudyID());
    fprintf(stderr, "Components: %d\n", reader->GetNumberOfComponents());
    fprintf(stderr, "Bits Allocated: %d\n", reader->GetBitsAllocated());
    fprintf(stderr, "Pixel rep: %d (%s)\n", reader->GetPixelRepresentation(), reader->GetPixelRepresentation() ? "signed" : "unsigned");
    fprintf(stderr, "Position: %g %g %g\n",
            reader->GetImagePositionPatient()[0],
            reader->GetImagePositionPatient()[1],
            reader->GetImagePositionPatient()[2]);
    fprintf(stderr, "Orientation: %g %g %g, %g %g %g\n",
            reader->GetImageOrientationPatient()[0], 
            reader->GetImageOrientationPatient()[1], 
            reader->GetImageOrientationPatient()[2], 
            reader->GetImageOrientationPatient()[3], 
            reader->GetImageOrientationPatient()[4], 
            reader->GetImageOrientationPatient()[5]);
    fprintf(stderr, "Rescale slope: %g, offset: %g\n", reader->GetRescaleSlope(), reader->GetRescaleOffset());
    fprintf(stderr, "Gantry angle: %g\n", reader->GetGantryAngle());
#endif

    Tcl_ListObjAppendList(interp, objPtr, metaDataObj);

    fprintf(stderr, "writing VTK\n");

    vtkSmartPointer<vtkDataSetWriter> writer = vtkSmartPointer<vtkDataSetWriter>::New();
    writer->SetInputConnection(reader->GetOutputPort());
    writer->SetFileTypeToBinary();
    writer->WriteToOutputStringOn();
    writer->Update();

    fprintf(stderr, "writing VTK...done\n");

    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj("vtkdata", -1));

    eltObj = Tcl_NewByteArrayObj((unsigned char *)writer->GetBinaryOutputString(),
                                 writer->GetOutputStringLength());
    Tcl_ListObjAppendElement(interp, objPtr, eltObj);

    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

extern "C" {
/*
 * ------------------------------------------------------------------------
 *  RpDicomToVtk_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "DicomToVtk" command into the interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpDicomToVtk_Init(Tcl_Interp *interp)
{
    /* install the widget command */
    Tcl_CreateObjCommand(interp, "Rappture::DicomToVtk", DicomToVtkCmd,
                         NULL, NULL);
    return TCL_OK;
}

}
