#ifndef MVERIFICATIONUTLITYFUNCTIONS_H_
#define MVERIFICATIONUTLITYFUNCTIONS_H_
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "dataAndTime.hpp"
#include <math.h>
#include <complex>

template <typename tt_dType>
tt_dType abs(std::complex<tt_dType> inSample) {
    double real = (tt_dType)inSample.real();
    double imag = (tt_dType)inSample.real();
    double result = sqrt((real * real + imag * imag));
}

template <typename tt_type, int L>
int exportArrayToFile(std::complex<tt_type> dataArray[L], std::string fileName) {
    int flag = 0;
    std::ofstream outFile;
    const char* tempFileName = fileName.c_str();
    outFile.open(tempFileName, std::ios::out);
    if (outFile.is_open() == false) {
        std::cout << "Cannot Open :" << fileName << " For writing output data...." << std::endl;
        std::cout << "Exiting...." << std::endl;
        flag = 1;
    }
    outFile << L << std::endl;
    for (int i = 0; i < L; i++) {
        outFile << dataArray[i].real() << std::endl;
        outFile << dataArray[i].imag() << std::endl;
    }
    return flag;
}

template <typename tt_type, int rows, int cols>
int export2DArrayToFile(std::complex<tt_type> dataArray[rows][cols], std::string fileName) {
    int flag = 0;
    std::ofstream outFile;
    const char* tempFileName = fileName.c_str();
    outFile.open(tempFileName, std::ios::out);
    std::cout << "\n----------------File Export Message --------------\n";
    if (outFile.is_open() == false) {
        std::cout << "Cannot Open :" << fileName << " For writing output data...." << std::endl;
        std::cout << "Exiting...." << std::endl;
        flag = 1;
    }
    outFile << cols << std::endl;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            outFile << dataArray[i][j].real() << std::endl;
            outFile << dataArray[i][j].imag() << std::endl;
        }
    }
    if (flag == 0) std::cout << "\n--------------File Exported Successfully----------\n";
    std::cout << "The export path is::" << fileName << "\n";
    return flag;
}

template <typename tt_type, int rows, int cols>
int exportArrayToFile(std::complex<tt_type> dataArray[rows][cols], std::string fileName) {
    std::ofstream outFile;
    const char* tempFileName = fileName.c_str();
    outFile.open(tempFileName, std::ios::out);
    if (outFile.is_open() == false) {
        std::cout << "Cannot Open :" << fileName << " For writing output data...." << std::endl;
        std::cout << "Exiting...." << std::endl;
        exit(1);
    }
    outFile << rows * cols << std::endl;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            outFile << dataArray[i][j].real() << std::endl;
            outFile << dataArray[i][j].imag() << std::endl;
        }
    }
}

// This function will read a file and compare it with expected output for functional verification
// It will print max error, min error, and mean square error for any given comparison
// It will also print log information in a file passed through ofstream
template <typename tt_dType>
int verifyArrayOutput(std::ofstream& logStream,
                      std::string strArrayName,
                      std::string fiofLog,
                      std::complex<tt_dType> inData[],
                      tt_dType maxAllowedABSError,
                      int percentageAllowedError,
                      int dataLength) {
    logStream << currentDateTime() << std::endl;
    std::cout << currentDateTime() << std::endl;
    logStream << "####################################---" << strArrayName
              << "---#######################################" << std::endl;
    std::cout << "####################################---" << strArrayName
              << "---#######################################" << std::endl;
    logStream << "fiofVerificationStart:" << strArrayName << std::endl
              << "Verifying Array:" << strArrayName << std::endl;
    std::cout << "fiofVerificationStart:" << strArrayName << std::endl
              << "Verifying Array:" << strArrayName << std::endl;
    logStream << "Starting Verification..." << std::endl;
    std::cout << "Starting Verification..." << std::endl;
    std::fstream fiofFile;
    logStream << "Trying to Open :" << fiofLog << std::endl;
    std::cout << "Trying to Open :" << fiofLog << std::endl;
    const char* fiofLogPtr = fiofLog.c_str();
    fiofFile.open(fiofLogPtr, std::ios_base::in);
    int veriFlag = 0;
    int verif_totalNumberOfErrors = 0;
    int sampleCompared = 0;
    int verif_status = 0;
    if (fiofFile.is_open()) /// Try to open the file for data verification
    {
        // Verification data file opened successfully start verification process

        // Verification Start
        logStream << "Opened verification data file :  " << fiofLog << " <----Successfully----->" << std::endl;
        std::cout << "Opened verification data file :  " << fiofLog << " <----Successfully----->..." << std::endl;
        // Variables to collect absolute error statistics /////////////////////////////////////////START
        double currentABSError;
        double maxABSError;
        double accumABSError = 0;
        // Variables to collect absolute error statistics /////////////////////////////////////////END

        // Variables to collect  error statistics of REAL PART ////////////////////////////////////START
        double currentREALError = 0;
        double maxREALError;
        double minREALError;
        double accumREALError = 0;
        // Variables to collect  error statistics of REAL PART //////////////////////////////////////END

        // Variables to collect  error statistics of IMAG PART ////////////////////////////////////START
        double currentIMAGError;
        double maxIMAGError;
        double minIMAGError;
        double accumIMAGError = 0;
        // Variables to collect  error statistics of IMAG PART //////////////////////////////////////END
        // complexNum<tt_dType> goldenSample;
        int dataLengthFromFile;
        if (fiofFile.eof()) // Reaing Array Length for Verification: The verification data in the file is not
                            // appropriate, less number of samples
        {
            logStream << "The data length in the verification file is less then the required no. of samples..."
                      << std::endl
                      << "Exiting..." << std::endl;
            std::cout << "The data length in the verification file is less then the required no. of samples..."
                      << std::endl
                      << "Exiting..." << std::endl;
            veriFlag = 1;
        } else // Array Length for Verfication is available for reading... /d
        {
            fiofFile >> dataLengthFromFile;
            if (dataLengthFromFile != dataLength) {
                logStream
                    << "The data length in the file does not matches the data array length passed for verification..."
                    << std::endl
                    << "Exiting..." << std::endl;
                std::cout
                    << "The data length in the file does not matches the data array length passed for verification..."
                    << std::endl
                    << "Exiting..." << std::endl;
                veriFlag = 1;
            } else // d
            {
                for (int i = 0; i < dataLength; i++) {
                    if (fiofFile.eof()) // The verification data in the file is not appropriate, less number of samples
                    {
                        logStream
                            << "The data length in the verification file is less then the required no. of samples..."
                            << std::endl
                            << "Exiting..." << std::endl;
                        std::cout
                            << "The data length in the verification file is less then the required no. of samples..."
                            << std::endl
                            << "Exiting..." << std::endl;
                        veriFlag = 1;
                        break;
                    } else // File opened and verification data available
                    {
                        double goldenSample_real;
                        double goldenSample_imag;
                        fiofFile >> goldenSample_real; // goldenSample.real;
                        fiofFile >> goldenSample_imag; // goldenSample.imag;
                        // goldenSample = std::complex<tt_dType>(temp_real,temp_imag);
                        // complexNum<tt_dType> temp  = inData[i] - goldenSample;
                        double inData_real, inData_imag;

                        inData_real = inData[i].real();
                        inData_imag = inData[i].imag();
                        currentREALError = inData_real - goldenSample_real;
                        currentIMAGError = inData_imag - goldenSample_imag;

                        currentABSError =
                            sqrt(currentIMAGError * currentIMAGError + currentREALError * currentREALError);
                        /// kepp track of max and min errors
                        if (i == 0) // first sample read for verification so init maximums and minimums
                        {
                            maxABSError = currentABSError;
                            maxREALError = currentREALError;
                            maxIMAGError = currentIMAGError;
                            minREALError = currentREALError;
                            minIMAGError = currentIMAGError;
                        } else {
                            if (currentABSError < 0) currentABSError *= -1;
                            // Check for maximum errors
                            if (currentABSError > maxABSError) maxABSError = currentABSError;
                            if (currentREALError > maxREALError) maxREALError = currentREALError;
                            if (currentIMAGError > maxIMAGError) maxIMAGError = currentIMAGError;
                            // Check for minimum errors
                            if (currentREALError < minREALError) minREALError = currentREALError;
                            if (currentIMAGError < minIMAGError) minIMAGError = currentIMAGError;
                            double golden_sample_abs =
                                sqrt(goldenSample_imag * goldenSample_imag + goldenSample_real * goldenSample_real);
                            double percent_error = (currentABSError / golden_sample_abs) * 100;
                            if (percent_error > maxAllowedABSError) {
                                logStream << "The data sample different significantly, comparing sample no:" << i
                                          << " The error is : " << percent_error << "%" << std::endl;
                                std::cout << "The data sample different significantly, comparing sample no:" << i
                                          << " The error is : " << percent_error << "%"
                                          << std::endl; //<< "The data sample different significantly, comparing sample
                                                        // no:"<<i<< std::endl;
                                // veriFlag=1;
                                verif_totalNumberOfErrors++;
                            }
                            accumABSError += currentABSError;
                            accumREALError += currentREALError;
                            accumIMAGError += currentIMAGError;
                        }
                    }
                    sampleCompared++;
                }
                int temp_percent_mismatch = int(double(verif_totalNumberOfErrors * 100) / double(dataLength));
                // verification successfull log statistics !!
                if (veriFlag == 0 && sampleCompared == dataLength && temp_percent_mismatch < percentageAllowedError) {
                    logStream << "Verification for data array" << strArrayName << " is SUCCCESSFUL !!" << std::endl;
                    std::cout << "Verification for data array" << strArrayName << " is SUCCCESSFUL !!" << std::endl;
                    verif_status = 0;
                } else {
                    logStream << "Verification for data array :" << strArrayName << " has FAILED !!" << std::endl;
                    std::cout << "Verification for data array : " << strArrayName << " has FAILED !!" << std::endl;
                    verif_status = 1;
                }
                //////////////////////////////////////////////////////////////////////////////////////
                logStream << "************************Error Rate is :" << temp_percent_mismatch
                          << " % ******************" << std::endl;
                std::cout << "************************Error Rate is :" << temp_percent_mismatch
                          << " % ******************" << std::endl;

                logStream << "************************Sample Mismatches :" << verif_totalNumberOfErrors
                          << " % ******************" << std::endl;
                std::cout << "************************Sample Mismatches :" << verif_totalNumberOfErrors
                          << " % ******************" << std::endl;

                logStream << "************************Maximum Error Numbers******************" << std::endl;
                std::cout << "************************Maximum Error Numbers******************" << std::endl;
                logStream << "The \"MAXIMUM ABSOLUTE ERROR\" : " << maxABSError << std::endl;
                std::cout << "The \"MAXIMUM ABSOLUTE ERROR\" : " << maxABSError << std::endl;

                logStream << "The \"MAXIMUM REAL ERROR\" : " << maxREALError << std::endl;
                std::cout << "The \"MAXIMUM REAL ERROR\" : " << maxREALError << std::endl;

                logStream << "The \"MAXIMUM IMAG ERROR\" : " << maxIMAGError << std::endl;
                std::cout << "The \"MAXIMUM IMAG ERROR\" : " << maxIMAGError << std::endl;

                //////////////////////////////////////////////////////////////////////////////////////
                logStream << "************************Average Error Numbers******************" << std::endl;
                std::cout << "************************Average Error Numbers******************" << std::endl;

                logStream << "The \"AVERAGE ABSOLUTE ERROR\" : " << (accumABSError / dataLength) << std::endl;
                std::cout << "The \"AVERAGE ABSOLUTE ERROR\" : " << (accumABSError / dataLength) << std::endl;

                logStream << "The \"AVERAGE REAL ERROR\" : " << (accumREALError / dataLength) << std::endl;
                std::cout << "The \"AVERAGE REAL ERROR\" : " << (accumREALError / dataLength) << std::endl;

                logStream << "The \"AVERAGE IMAG ERROR\" : " << (accumIMAGError / dataLength) << std::endl;
                std::cout << "The \"AVERAGE IMAG ERROR\" : " << (accumIMAGError / dataLength) << std::endl;

                //////////////////////////////////////////////////////////////////////////////////////
                logStream << "************************Minimum Error Numbers******************" << std::endl;
                std::cout << "************************Minimum Error Numbers******************" << std::endl;

                logStream << "The \"MINIMUM REAL  ERROR\" : " << minREALError << std::endl;
                std::cout << "The \"MINIMUM REAL  ERROR\" : " << minREALError << std::endl;

                logStream << "The \"MINIMUM IMAG  ERROR\" : " << minIMAGError << std::endl;
                std::cout << "The \"MINIMUM IMAG  ERROR\" : " << minIMAGError << std::endl;
            } //%%%
        }     ///%%%
    } else    // Cannot Open verification data File SUCCCESSFULLY Error out !!
    {
        logStream << "Cannot Open input data file for verification..." << std::endl << "Exiting..." << std::endl;
        std::cout << "Cannot Open input data file for verification..." << std::endl << "Exiting..." << std::endl;
        verif_status = 1;
    }

    return verif_status;
}

template <typename tt_dType>
int readComplexArrayFromFile(std::ofstream& logStream,
                             std::string strArrayName,
                             std::string fiofInputFileName,
                             std::complex<tt_dType> outData[],
                             int dataLength) {
    logStream << currentDateTime() << std::endl;
    std::cout << currentDateTime() << std::endl;
    logStream << "####################################---Error Statistics for :" << strArrayName
              << "---#######################################" << std::endl;
    logStream << "####################################---Error Statistics for :" << strArrayName
              << "---#######################################" << std::endl;
    logStream << "fiofVerificationReadInputData:" << strArrayName << std::endl
              << "Reading Array:" << strArrayName << std::endl;
    std::cout << "fiofVerificationReadInputData:" << strArrayName << std::endl
              << "Reading Array:" << strArrayName << std::endl;
    std::fstream fiofFile;
    logStream << "Trying to Open :" << fiofInputFileName << std::endl;
    std::cout << "Trying to Open :" << fiofInputFileName << std::endl;
    const char* fiofInputFileNamePtr = fiofInputFileName.c_str();
    fiofFile.open(fiofInputFileNamePtr, std::ios_base::in);
    int veriFlag = 0;
    if (fiofFile.is_open()) /// Try to open the file for data verification
    {
        logStream << "Opened input data file :  " << fiofInputFileName << " <----Successfully----->..." << std::endl;
        std::cout << "Opened input data file :  " << fiofInputFileName << " <----Successfully----->..." << std::endl;
        if (fiofFile.eof()) // Reaing Array Length for Verification: The verification data in the file is not
                            // appropriate, less number of samples
        {
            logStream << "The data length in the verification file:" << fiofInputFileName
                      << " is less then the required no. of samples..." << std::endl
                      << "Exiting..." << std::endl;
            std::cout << "The data length in the verification file:" << fiofInputFileName
                      << " is less then the required no. of samples..." << std::endl
                      << "Exiting..." << std::endl;
            veriFlag = 1;
        } else // Array Length for Verfication is available for reading... /d
        {
            int dataLengthFromFile;
            fiofFile >> dataLengthFromFile;
            if (dataLengthFromFile != dataLength) {
                logStream << "The data length in the file" << fiofInputFileName
                          << "does not matches the data array length passed for verification..." << std::endl
                          << "Exiting..." << std::endl;
                std::cout << "The data length in the file" << fiofInputFileName
                          << "does not matches the data array length passed for verification..." << std::endl
                          << "Exiting..." << std::endl;
                veriFlag = 1;
            } else // Array Length found appropriate now READ ALL the dat from FILE
            {
                for (int i = 0; i < dataLength; i++) {
                    // File opened data legnth is appropriate !!!
                    if (fiofFile.eof()) // The  data in the file is not appropriate, LESS number of samples
                    {
                        logStream << "The data length in the input data file" << fiofInputFileName
                                  << "is less then the required no. of samples..." << std::endl
                                  << "Exiting..." << std::endl;
                        std::cout << "The data length in the input data file" << fiofInputFileName
                                  << "is less then the required no. of samples..." << std::endl
                                  << "Exiting..." << std::endl;
                        veriFlag = 1;
                        break;
                    } else // File opened and verification data available
                    {
                        double temp_real, temp_imag;
                        fiofFile >> temp_real; // outData[i].real;
                        fiofFile >> temp_imag; // outData[i].imag;
                        outData[i] = std::complex<tt_dType>(temp_real, temp_imag);
                    }
                }
            }
        }

    } else // Cannot Open verification data File SUCCCESSFULLY Error out !!
    {
        logStream << "Cannot Open input data file for stimulus..." << fiofInputFileName << std::endl
                  << "Exiting..." << std::endl;
        std::cout << "Cannot Open input data file for stimulus..." << fiofInputFileName << std::endl
                  << "Exiting..." << std::endl;
        veriFlag = 1;
    }

    return veriFlag;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ This function will export an array to a file  : complexNum type Support
#if 0 //#if 0 // disable ComplexNum Support
template<typename tt_type,int L>
int exportArrayToFile(complexNum < tt_type> dataArray[L], std::string fileName)
{
	int flag=0;
	std::ofstream outFile;
	const char *tempFileName = fileName.c_str();
	outFile.open(tempFileName, std::ios::out);
	if (outFile.is_open() == false)
	{
		std::cout << "Cannnot Open :" << fileName << " For writing output data...." << std::endl;
		std::cout << "Exiting...." << std::endl;
		flag=1;
	}
	outFile << L << std::endl;
	for (int i = 0; i < L; i++)
	{
			
		outFile << dataArray[i].real << std::endl;
		outFile << dataArray[i].imag << std::endl;

	}
	return flag;
}
template<typename tt_type, int L,int R>
int exportArrayToFile(complexNum< tt_type> dataArray[][R], std::string fileName)
{
	std::ofstream outFile;
	const char *tempFileName = fileName.c_str();
	outFile.open(tempFileName, std::ios::out);
	if (outFile.is_open() == false)
	{
		std::cout << "Cannnot Open :" << fileName << " For writing output data...." << std::endl;
		std::cout << "Exiting...." << std::endl;
		exit(1);
	}
	outFile << L*R << std::endl;
	for (int i = 0; i < L; i++)
	{
		for (int j = 0; j < R; j++)
		{
			outFile << dataArray[i][j].real << std::endl;
			outFile << dataArray[i][j].imag << std::endl;


		}
	}

}

// This function will read a file and compare it with expected output for functional verification 
// It will print max error, min error, and mean square error for any given comparison
// It will also print log information in a file passed through ofstream

template < typename tt_dType >
int verifyArrayOutput(std::ofstream& logStream, std::string strArrayName, std::string fiofLog, complexNum<tt_dType> inData[], tt_dType maxAllowedABSError,int dataLength)
{
	logStream << currentDateTime() << std::endl;
	std::cout<< currentDateTime() << std::endl;
	logStream << "####################################---"<<strArrayName<< "---#######################################" << std::endl;
	std::cout << "####################################---" << strArrayName << "---#######################################" << std::endl;
	logStream << "fiofVerificationStart:"<<strArrayName<<std::endl<<"Verifying Array:" << strArrayName << std::endl;
	std::cout << "fiofVerificationStart:"<<strArrayName<<std::endl<<"Verifying Array:" << strArrayName << std::endl;
	logStream << "Starting Verfication..." << std::endl;
	std::cout << "Starting Verfication..." << std::endl;
	std::fstream fiofFile;
	logStream << "Trying to Open :" << fiofLog << std::endl;
	std::cout << "Trying to Open :" << fiofLog << std::endl;
	const char *fiofLogPtr = fiofLog.c_str();
	fiofFile.open(fiofLogPtr, std::ios_base::in);
	int veriFlag = 0;
	if (fiofFile.is_open()) /// Try to open the file for data verification
	{
		// Verification data file opened successfully start verification process
		
		// Verification Start
		logStream << "Opened verfication data file :  " << fiofLog << " <----Successfully----->" << std::endl;
		std::cout << "Opened verfication data file :  " << fiofLog << " <----Successfully----->..." << std::endl;
		// Variables to collect absolute error statistics /////////////////////////////////////////START
		tt_dType currentABSError;
		tt_dType maxABSError;
		tt_dType accumABSError = 0;
		// Variables to collect absolute error statistics /////////////////////////////////////////END

		// Variables to collect  error statistics of REAL PART ////////////////////////////////////START
		tt_dType currentREALError = 0;
		tt_dType maxREALError;
		tt_dType minREALError;
		tt_dType accumREALError = 0;
		// Variables to collect  error statistics of REAL PART //////////////////////////////////////END

		// Variables to collect  error statistics of IMAG PART ////////////////////////////////////START
		tt_dType currentIMAGError;
		tt_dType maxIMAGError;
		tt_dType minIMAGError;
		tt_dType accumIMAGError = 0;
		// Variables to collect  error statistics of IMAG PART //////////////////////////////////////END
		complexNum<tt_dType> goldenSample;
		int dataLengthFromFile;
		if (fiofFile.eof()) // Reaing Array Length for Verification: The verification data in the file is not appropriate, less number of samples
		{
			logStream << "The data length in the verification file is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
			std::cout << "The data length in the verification file is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
			veriFlag=1;
		}
		else // Array Length for Verfication is available for reading... /d
		{	
				fiofFile >> dataLengthFromFile;
				if(dataLengthFromFile!=dataLength)
				{
					logStream << "The data length in the file does not matches the data array length passed for verification..." << std::endl << "Exiting..." << std::endl;
					std::cout << "The data length in the file does not matches the data array length passed for verification..." << std::endl << "Exiting..." << std::endl;
					veriFlag=1;
				}
				else //d
				{		
					for (int i = 0; i <dataLength; i++)
					{
						if (fiofFile.eof()) // The verification data in the file is not appropriate, less number of samples
						{

							logStream << "The data length in the verification file is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
							std::cout << "The data length in the verification file is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
							veriFlag=1;
							break;
						}
						else // File opened and verification data available 
						{
										fiofFile >> goldenSample.real;
										fiofFile >> goldenSample.imag;
										complexNum<tt_dType> temp  = inData[i] - goldenSample;
										currentABSError = temp.cabs();
										currentREALError = temp.real;
										currentIMAGError = temp.imag;
										///kepp track of max and min errors
										if (i == 0) // first sample read for verification so init maximums and minimums
										{
											maxABSError = currentABSError;
											maxREALError = temp.real;
											maxIMAGError = temp.imag;
											minREALError = temp.real;
											minIMAGError = temp.imag;
										}
										else
										{	
											if(currentABSError < 0)
												currentABSError*= -1;
											// Check for maximum errors
											if(currentABSError > maxABSError)
												maxABSError=currentABSError;
											if(currentREALError > maxREALError)
												maxREALError=currentREALError;
											if(currentIMAGError > maxIMAGError)
												maxIMAGError=currentIMAGError;
											// Check for minimum errors					
											if(currentREALError < minREALError)
												minREALError=currentREALError;
											if(currentIMAGError < minIMAGError)
												minIMAGError=currentIMAGError;
											
											if(currentABSError > maxAllowedABSError)
											{
												logStream << "The data sample different signigicantly, comapring sample no:" <<i<< std::endl;
												std::cout << "The data sample different signigicantly, comapring sample no:"<<i<< std::endl;
												veriFlag=1;
											}
											accumABSError += currentABSError;
											accumREALError+=currentREALError;
											accumIMAGError+=currentIMAGError;
										}
							}				
					}
					// verification successfull log statistics !!
					if (veriFlag==0)
					{
						logStream << "Verification for data array" << strArrayName << "is SUCCCESSFUL !!" << std::endl;
						std::cout << "Verification for data array" << strArrayName << "is SUCCCESSFUL !!" << std::endl;
					}
					else
					{
						logStream << "Verification for data array :" << strArrayName << "is FAILED !!" << std::endl;
						std::cout << "Verification for data array : " << strArrayName << "is FAILED !!" << std::endl;
					}
					//////////////////////////////////////////////////////////////////////////////////////
					logStream<<"************************Maximum Error Numbers******************"<<std::endl;
					std::cout	 <<"************************Maximum Error Numbers******************"<<std::endl;
					logStream<<"The \"MAXIMUM ABSOLUTE ERROR\" : "<<maxABSError<<std::endl;
					std::cout     <<"The \"MAXIMUM ABSOLUTE ERROR\" : "<<maxABSError<<std::endl;

					logStream<<"The \"MAXIMUM REAL ERROR\" : "<<maxREALError<<std::endl;
					std::cout     <<"The \"MAXIMUM REAL ERROR\" : "<<maxREALError<<std::endl;
					
					logStream<<"The \"MAXIMUM IMAG ERROR\" : "<<maxIMAGError<<std::endl;
					std::cout     <<"The \"MAXIMUM IMAG ERROR\" : "<<maxIMAGError<<std::endl;
					
					
					//////////////////////////////////////////////////////////////////////////////////////
					logStream<<"************************Average Error Numbers******************"<<std::endl;
					std::cout	 <<"************************Average Error Numbers******************"<<std::endl;		
					
					logStream<<"The \"AVERAGE ABSOLUTE ERROR\" : "<<(accumABSError/dataLength)<<std::endl;
					std::cout     <<"The \"AVERAGE ABSOLUTE ERROR\" : "<<(accumABSError/dataLength)<<std::endl;

					logStream<<"The \"AVERAGE REAL ERROR\" : "<<(accumREALError/dataLength)<<std::endl;
					std::cout	 <<"The \"AVERAGE REAL ERROR\" : "<<(accumREALError/dataLength)<<std::endl;
					
					logStream<<"The \"AVERAGE IMAG ERROR\" : "<<(accumIMAGError/dataLength)<<std::endl;
					std::cout <<"The \"AVERAGE IMAG ERROR\" : "<<(accumIMAGError/dataLength)<<std::endl;

					//////////////////////////////////////////////////////////////////////////////////////
					logStream<<"************************Minimum Error Numbers******************"<<std::endl;
					std::cout	 <<"************************Minimum Error Numbers******************"<<std::endl;		

					logStream<<"The \"MINIMUM REAL  ERROR\" : "<<minREALError<<std::endl;
					std::cout	 <<"The \"MINIMUM REAL  ERROR\" : "<<minREALError<<std::endl;
					
					logStream<<"The \"MINIMUM IMAG  ERROR\" : "<<minIMAGError<<std::endl;
					std::cout	 <<"The \"MINIMUM IMAG  ERROR\" : "<<minIMAGError<<std::endl;
				} //%%%
		}///%%%
	}
	else // Cannot Open verification data File SUCCCESSFULLY Error out !!
	{
		logStream << "Cannot Open input data file for verfication..." << std::endl << "Exiting..." << std::endl;
		std::cout << "Cannot Open input data file for verfication..." << std::endl << "Exiting..." << std::endl;
		veriFlag=1;
	}
	return veriFlag;
}
template <typename tt_dType>
int readComplexArrayFromFile(std::ofstream& logStream, std::string strArrayName, std::string fiofInputFileName, complexNum<tt_dType> outData[], int dataLength)
{
	logStream << currentDateTime() << std::endl;
	std::cout << currentDateTime() << std::endl;
	logStream << "####################################---" << strArrayName << "---#######################################" << std::endl;
	std::cout << "####################################---" << strArrayName << "---#######################################" << std::endl;
	logStream << "fiofVerificationReadInputData:" << strArrayName << std::endl << "Reading Array:" << strArrayName << std::endl;
	std::cout << "fiofVerificationReadInputData:" << strArrayName << std::endl << "Reading Array:" << strArrayName << std::endl;
	std::fstream fiofFile;
	logStream << "Trying to Open :" << fiofInputFileName << std::endl;
	std::cout << "Trying to Open :" << fiofInputFileName << std::endl;
	const char *fiofInputFileNamePtr = fiofInputFileName.c_str();
	fiofFile.open(fiofInputFileNamePtr, std::ios_base::in);
	int veriFlag = 0;
	if (fiofFile.is_open()) /// Try to open the file for data verification
	{
		logStream << "Opened input data file :  " << fiofInputFileName << " <----Successfully----->..." << std::endl;
		std::cout << "Opened input data file :  " << fiofInputFileName << " <----Successfully----->..." << std::endl;
		if (fiofFile.eof()) // Reaing Array Length for Verification: The verification data in the file is not appropriate, less number of samples
		{
			logStream << "The data length in the verification file:" << fiofInputFileName << " is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
			std::cout << "The data length in the verification file:" << fiofInputFileName << " is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
			veriFlag = 1;
		}
		else // Array Length for Verfication is available for reading... /d
		{
			int dataLengthFromFile;
			fiofFile >> dataLengthFromFile;
			if (dataLengthFromFile != dataLength)
			{
				logStream << "The data length in the file" << fiofInputFileName << "does not matches the data array length passed for verification..." << std::endl << "Exiting..." << std::endl;
				std::cout << "The data length in the file" << fiofInputFileName << "does not matches the data array length passed for verification..." << std::endl << "Exiting..." << std::endl;
				veriFlag = 1;
			}
			else // Array Length found appropriate now READ ALL the dat from FILE
			{
				for (int i = 0; i < dataLength; i++)
				{
					// File opened data legnth is appropriate !!!
					if (fiofFile.eof()) // The  data in the file is not appropriate, LESS number of samples
					{

						logStream << "The data length in the input data file" << fiofInputFileName << "is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
						std::cout << "The data length in the input data file" << fiofInputFileName << "is less then the required no. of samples..." << std::endl << "Exiting..." << std::endl;
						veriFlag = 1;
						break;
					}
					else // File opened and verification data available 
					{
						fiofFile >> outData[i].real;
						fiofFile >> outData[i].imag;
					}
				}
			}
		}

	}
	else // Cannot Open verification data File SUCCCESSFULLY Error out !!
	{
		logStream << "Cannot Open input data file for stimulus..." << fiofInputFileName<< std::endl << "Exiting..." << std::endl;
		std::cout << "Cannot Open input data file for stimulus..." << fiofInputFileName<< std::endl << "Exiting..." << std::endl;
		veriFlag = 1;
	}

	return veriFlag;
}
#endif

template <int size_dim1, int size_dim2, int offset, int dim, typename tt_dtype>
bool verifyContineous2DArray(tt_dtype data[size_dim1][size_dim2]) {
    if (dim == 1) {
        tt_dtype num = offset;
        for (int d2 = 0; d2 < size_dim2; d2++) {
            for (int d1 = 0; d1 < size_dim1; d1++) {
                if (num != data[d1][d2])
                    return false;
                else
                    num++;
            }
        }
    } else if (dim == 2) {
        tt_dtype num = offset;
        for (int d1 = 0; d1 < size_dim1; d1++) {
            for (int d2 = 0; d2 < size_dim2; d2++) {
                if (num != data[d1][d2])
                    return false;
                else
                    num++;
            }
        }
    } else
        return false;

    return true;
}

#endif // !MVERIFICATIONUTLITYFUNCTIONS_H_
