/**************************************************************************
 *  SPARSE_GRIDDING_DISTANCE_MEX.C
 *
 *  Author: Scott Haile Robertson
 *  Website: www.ScottHaileRobertson.com
 *  Date: June 4, 2012
 *
 *  A MATLAB mex wrapper for an N-Dimmensional gridding helper.
 *  Motivated by code written by Gary glover
 *  (http://www-mrsrl.stanford.edu/~brian/gridding/)
 *  code by Nick Zwart, and code by Jeff Fessler ()
 *
 *  For background reading, I suggest:
 *      1. A fast Sinc Function Gridding Algorithm for Fourier Inversion in
 *         Computer Tomography. O'Sullivan. 1985.
 *      2. Selection of a Convolution Function for Fourier Inversion using
 *         Gridding. Jackson et al. 1991.
 *      3. Rapid Gridding Reconstruction With a Minimal Oversampling Ratio.
 *         Beatty et al. 2005.
 *
 *  NOTE #1: You must compile with the -largeArrayDims option to enable
 *           the creation of sparse matrices
 *
 *  NOTE #2: compiling in debug mode (mex -g grid_conv_mex.c) turns on error
 *           checking of inputs. I recommend that you compile in debug mode
 *           to ensure that you are passing input arguments correctly, then
 *           recompile without debug mode once you have it wired correctly and
 *           enjoy the speed.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This code
 *  is for research and academic purposes and is not intended for
 *  clinical use.
 *
 **************************************************************************/
#include "mex.h"
#include "sparse_gridding_distance.c"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
	
	/* Variable declarations */
	double *coords;                     // Array of coordinates (each column is a dimmension)
	double kernel_width;                // Convolution kernel width
	unsigned int *output_dims;    // Size of output dimensions
	const mwSize *dims;           // Dimension vector [#pts, #dimmensions]
	unsigned int ndims;                 // Number of dimensions sampled
	mwSize npts;                  // Total number of points sampled
	mwSize nvoxels = 1;
	mwSize max_n_neighbors = 1; // Total number of nonzero entries
	unsigned int dim;
	double *sparse_distances;
    double *sparse_sample_indices;
    double *sparse_first_nonzero_voxel;
	mwSignedIndex tempSize[2];
	
	/* FOR DEBUG ONLY */
// #define DEBUG 1;
#ifdef DEBUG
	unsigned int i;
	int dim2;
#endif
	
	/* CHECK INPUT ARGUMENT COUNT */
	if (nrhs != 3) {
		mexErrMsgTxt("Required inputs: coords, kernel_width, and output_dims.");
	}
	
// 	/* CHECK OUTPUT ARGUMENT COUNT *
// 	 * REQUIRED: grid_volume  */
// 	if (nlhs != 1) {
// 		mexErrMsgTxt("Exactly 1 output (sparse distance volume) is returned.");
// 	}
	
	/* INPUT 0 - COORDINATES - can be 2D, 3D, 4D, and beyond */
	mxAssert(!mxIsEmpty(prhs[0]),"coords cannot be null.");
	mxAssert(mxIsDouble(prhs[0]), "coords must be of type double");
	mxAssert(mxGetNumberOfDimensions(prhs[0]) == 2, "coords must be a 2D array (each dimmension is a column)");
	dims =  mxGetDimensions(prhs[0]);           // get dimension vector
	ndims = (unsigned int) dims[0];                             // number of dimensions
	npts  = (unsigned int) dims[1];                             // number of points
	coords = mxGetPr(prhs[0]);                                  // coordinates
	printf("Ndims=%u, npts=%u\n",ndims,npts);
	
	/* INPUT 1 - KERNEL WIDTH */
	mxAssert(!mxIsEmpty(prhs[1]),"kernel_width cannot be null.");
	mxAssert(mxIsDouble(prhs[1]), "kernel_width must be of type double.");
	mxAssert(mxGetNumberOfDimensions(prhs[1]) == 2, "kernel_width has wrong number of dimmensions (should be just a double value).");
	mxAssert(mxGetDimensions(prhs[1])[0] == 1, "kernel_width must be a double value, not an array.");
	mxAssert(mxGetDimensions(prhs[1])[1] == 1, "kernel_width must be a double value, not an array.");
	kernel_width = *mxGetPr(prhs[1]);                           // kernel_width
	
	/* INPUT 2 - OUTPUT VOLUME DIMS - Used to create output volume */
	mxAssert(!mxIsEmpty(prhs[2]),"vol_dims cannot be null.");
	mxAssert(mxGetNumberOfDimensions(prhs[2]) == 2, "vol_dims has the wrong number of dimmensions (should be a row vector).");
	mxAssert(mxGetDimensions(prhs[2])[0] == ndims, "there must be the same number of vol_dims as dimmensions.");
	mxAssert(mxGetDimensions(prhs[2])[1] == 1, "vol_dims must be a column vector.");
	output_dims = (unsigned int *) mxGetPr(prhs[2]);            // output dimensions
	
	// CALCULATE TOTAL NUMBER OF VOXELS AND NEIGHBORS
	for(dim=0; dim<ndims; dim++){
		nvoxels = nvoxels*output_dims[dim];
		max_n_neighbors = max_n_neighbors*kernel_width;
	}
	
		/* DEBUG PRINTING */
#ifdef DEBUG
	mexPrintf("ndims=%i\n",ndims);
	mexPrintf("npts=%i\n\n",npts);
	
	mexPrintf("Input coords:\n");
	for(i=0; i<npts; i++){
		mexPrintf("\tCoords[%u]=(%f,%f,%f)\n",i,coords[3*i],coords[3*i+1],coords[3*i+2]);
	}
	mexPrintf("End of input coords.\n\n");
	
	mexPrintf("Kernel width=:%f\n\n",kernel_width);
	
	mexPrintf("Input output_dims:\n");
	for(i=0; i<ndims; i++){
		mexPrintf("\toutput_dims[%u]=%u\n",i,output_dims[i]);
	}
	mexPrintf("End of ouput_dims.\n\n");

	printf("max nonzero=%u\n",npts*max_n_neighbors);
#endif
	
// 	/* OUTPUT 0 - GRIDDED KSPACE - Created with dimensions of output_dims */
//  	plhs[0] = mxCreateSparse(npts,nvoxels,npts*max_n_neighbors,mxREAL);
//     sparse_distances  = mxGetPr(plhs[0]);v
//     sparse_sample_indices = mxGetIr(plhs[0]);
//     sparse_first_nonzero_voxel = mxGetJc(plhs[0]);
	
	tempSize[0] = round(npts*max_n_neighbors);
	tempSize[1] = 1;
	plhs[0] = mxCreateNumericArray(2,tempSize,mxDOUBLE_CLASS,mxREAL); // output
    mxAssert(!mxIsEmpty(plhs[0]),"Sample index matrix was not allocated correctly. Possibly out of memory.");
    sparse_sample_indices = mxGetPr(plhs[0]);

	plhs[1] = mxCreateNumericArray(2,tempSize,mxDOUBLE_CLASS,mxREAL); // output
    mxAssert(!mxIsEmpty(plhs[1]),"Voxel index matrix was not allocated correctly. Possibly out of memory.");
    sparse_first_nonzero_voxel = mxGetPr(plhs[1]);

	plhs[2] = mxCreateNumericArray(2,tempSize,mxDOUBLE_CLASS,mxREAL); // output
    mxAssert(!mxIsEmpty(plhs[2]),"Distance matrix was not allocated correctly. Possibly out of memory.");
    sparse_distances = mxGetPr(plhs[2]);

	
	/* Perform the convolution based gridding */
	sparse_gridding_distance(coords, kernel_width, npts, ndims, max_n_neighbors, output_dims,
			sparse_sample_indices, sparse_first_nonzero_voxel, sparse_distances);
}


