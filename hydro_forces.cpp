#include "hydro_forces.h"
// =============================================================================

/*******************************************************************************
* BodyFileInfo::read_data()
* private member function called from constructor 
* reads h5 file data and stores it in member variables for use with other
* classes and forces
*******************************************************************************/
void BodyFileInfo::read_data() {
	// testing HDF5 compatibility
	// open file with read only access
	H5::H5File sphereFile(h5_file_name, H5F_ACC_RDONLY); // "../../test_for_chrono/sphere.h5"

	//
	// Read linear restoring stiffness file info into matrices
	// "body1"
	std::string data_name = bodyNum + "/hydro_coeffs/linear_restoring_stiffness"; // "body1/hydro_coeffs/linear_restoring_stiffness"
	H5::DataSet dataset = sphereFile.openDataSet(data_name);
	// Get filespace for rank and dimension
	H5::DataSpace filespace = dataset.getSpace();
	// Get number of dimensions in the file dataspace
	// Get and print the dimension sizes of the file dataspace
	hsize_t dims[2];    // dataset dimensions
	int rank = filespace.getSimpleExtentDims(dims);
	// read file into data_out 2d array
	H5::DataSpace mspace1(rank, dims);
	double *temp; 
	temp = new double[dims[0] * dims[1]];
	// read file info into data_out, a 2d array
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	// turn the 2d array into a ChMatrix (Eigen dynamic matrix)
	lin_matrix.resize(dims[0], dims[1]); 
	for (int i = 0; i < dims[0]; i++) {
		for (int j = 0; j < dims[1]; j++) {
			lin_matrix(i, j) = temp[i * dims[1] + j]; 
		}
	}
	dataset.close();
	delete[] temp;

	data_name = bodyNum + "/hydro_coeffs/added_mass/inf_freq";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	rank = filespace.getSimpleExtentDims(dims);
	mspace1 = H5::DataSpace(rank, dims);
	temp = new double[dims[0] * dims[1]];
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	// put into equil chvector
	inf_freq.resize(dims[0], dims[1]);
	for (int i = 0; i < dims[0]; i++) {
		for (int j = 0; j < dims[1]; j++) {
			inf_freq(i, j) = temp[i * dims[1] + j];
		}
	}
	dataset.close();
	delete[] temp;

	// repeat same steps from above to get the cb and cg...reusing some of the previous arrays etc
	data_name = bodyNum + "/properties/cb";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	rank = filespace.getSimpleExtentDims(dims);
	mspace1 = H5::DataSpace(rank, dims);
	temp = new double[dims[0] * dims[1]];
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	// put into equil chvector
	for (int i = 0; i < dims[0]; i++) {
		cb[i] = temp[i];
	}
	dataset.close();

	// repeat finally for cg
	data_name = bodyNum + "/properties/cg";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	rank = filespace.getSimpleExtentDims(dims);
	mspace1 = H5::DataSpace(rank, dims);
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	// put into equil chvector
	for (int i = 0; i < dims[0]; i++) {
		cg[i] = temp[i];
	}
	dataset.close();
	delete[] temp;
	
	// read displaced volume for buoyancy force
	data_name = bodyNum + "/properties/disp_vol";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	rank = filespace.getSimpleExtentDims(dims);
	mspace1 = H5::DataSpace(rank, dims);
	temp = new double[dims[0] * dims[1]];
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	disp_vol = temp[0];
	dataset.close();

	// read rho
	data_name = "simulation_parameters/rho";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	rank = filespace.getSimpleExtentDims(dims);
	mspace1 = H5::DataSpace(rank, dims);
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	rho = temp[0];
	dataset.close();

	// read g
	data_name = "simulation_parameters/g";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	rank = filespace.getSimpleExtentDims(dims);
	mspace1 = H5::DataSpace(rank, dims);
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	g = temp[0];
	dataset.close();
	delete[] temp;
	lin_matrix *= rho*g; // scale by rho*g

	// read K
	data_name = bodyNum + "/hydro_coeffs/radiation_damping/impulse_response_fun/K";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	int rank3 = filespace.getSimpleExtentDims(K_dims);
	// read file into data_out 2d array
	H5::DataSpace mspace3(rank3, K_dims);
	// K_dims[0] is number of rows, K_dims[1] is number of columns, K_dims[2] is number of matrices
	K_matrix = new double[K_dims[0] * K_dims[1] * K_dims[2]];
	// read file info into data_out, a 2d array
	dataset.read(K_matrix, H5::PredType::NATIVE_DOUBLE, mspace3, filespace);
	dataset.close();

	// read timesteps vector
	data_name = bodyNum + "/hydro_coeffs/radiation_damping/impulse_response_fun/t";
	dataset = sphereFile.openDataSet(data_name);
	filespace = dataset.getSpace();
	rank = filespace.getSimpleExtentDims(dims);
	mspace1 = H5::DataSpace(rank, dims);
	temp = new double[dims[0] * dims[1]];
	timesteps.resize(dims[0]);
	dataset.read(temp, H5::PredType::NATIVE_DOUBLE, mspace1, filespace);
	// put into timesteps chvector
	for (int i = 0; i < dims[0]; i++) {
		timesteps[i] = temp[i];
	}

	dataset.close();
	delete[] temp;

	sphereFile.close();
}

BodyFileInfo::BodyFileInfo() {}

BodyFileInfo::~BodyFileInfo() {
	delete[] K_matrix;
}

/*******************************************************************************
* BodyFileInfo constructor
* requires file name (in absolute file name or referenced from executable location)
* and body name (name of body's section in H5 file, ie "body1" etc)
* each body in system should have its own BodyFileInfo object
* calls read_data()
*******************************************************************************/
BodyFileInfo::BodyFileInfo(std::string file, std::string bodyName) {
	h5_file_name = file;
	bodyNum = bodyName;
	read_data();
}

/*******************************************************************************
* BodyFileInfo::get_lin_matrix()
* returns the linear restoring stiffness matrix 
*******************************************************************************/
ChMatrixDynamic<double> BodyFileInfo::get_lin_matrix() const {
	return lin_matrix;
}

/*******************************************************************************
* BodyFileInfo::get_added_mass_matrix()
* returns the added mass matrix at infinite frequency
*******************************************************************************/
ChMatrixDynamic<double> BodyFileInfo::get_added_mass_matrix() const {
	return inf_freq * rho;
}

/*******************************************************************************
* BodyFileInfo::get_equil_cg()
* returns cg, center of gravity of object's body
*******************************************************************************/
ChVector<> BodyFileInfo::get_equil_cg() const {
	return cg;
}

/*******************************************************************************
* BodyFileInfo::get_equil_cb()
* returns cb, the rotational equilibrium
*******************************************************************************/
ChVector<> BodyFileInfo::get_equil_cb() const {
	return cb;
}

/*******************************************************************************
* BodyFileInfo::get_rho()
* returns the density of water, rho (kg/m^3 usually)
*******************************************************************************/
double BodyFileInfo::get_rho() const {
	return rho;
}

/*******************************************************************************
* BodyFileInfo::get_g()
* returns g, gravitational acceleration, m/s^2
*******************************************************************************/
double BodyFileInfo::get_g() const {
	return g;
}

/*******************************************************************************
* BodyFileInfo::get_disp_vol() 
* returns displaced volume when body at equilibrium, m^3
*******************************************************************************/
double BodyFileInfo::get_disp_vol() const {
	return disp_vol;
}

/*******************************************************************************
* BodyFileInfo::get_impulse_resp_matrix()
* returns impulse response coeff for row m, column n, step s
*******************************************************************************/
double BodyFileInfo::get_impulse_resp(int m, int n, int s) const {
	int index = s + K_dims[2] * (n + m * K_dims[1]);
	if (index < 0 || index >= K_dims[0] * K_dims[1] * K_dims[2]) {
		std::cout << "out of bounds IRF\n";
		return 0;
	}
	else {
		return K_matrix[index] * get_rho(); // scale radiation force by rho
	}
}

/*******************************************************************************
* BodyFileInfo::get_K_dims(int i) returns the i-th component of the dimensions of K_matrix
* i = [0,1,2] -> [number of rows, number of columns, number of matrices]
*******************************************************************************/
int BodyFileInfo::get_K_dims(int i) const {
	return K_dims[i];
}


/*******************************************************************************
* BodyFileInfo::get_delta_t() returns the difference in first 2 timesteps
*******************************************************************************/
double BodyFileInfo::get_delta_t() const {
	return timesteps[1] - timesteps[0];
}
/*******************************************************************************
* BodyFileInfo::get_times()
* returns the vector of timesteps from h5 file
*******************************************************************************/
std::vector<double> BodyFileInfo::get_times() const {
	return timesteps;
}

// =============================================================================

/*******************************************************************************
* ForceTorqueFunc constructor
* sets pointer to LinRestorForce member object and index for which component
* this ForceTorqueFunc object refers to
*******************************************************************************/
ForceTorqueFunc::ForceTorqueFunc(LinRestorForce* b, int i) : base(b), index(i) {	}

/*******************************************************************************
* ForceTorqueFunc::Clone()
* required override function since ForceTorqueFunc inherits from ChFunction
*******************************************************************************/
ForceTorqueFunc* ForceTorqueFunc::Clone() const  { 
	return new ForceTorqueFunc(*this); 
}

/*******************************************************************************
* ForceTorqueFunc::Get_y()
* returns the value of the index-th coordinate of the linear restoring force
* at each time
* required override function since ForceTorqueFunc inherits from ChFunction
*******************************************************************************/
double ForceTorqueFunc::Get_y(double x) const {
	return base->coordinateFunc(index);
}

/*******************************************************************************
* ForceTorqueFunc::SetBase
* currently unused(?) function to set base if not set in constructor
*******************************************************************************/
void ForceTorqueFunc::SetBase(LinRestorForce* b) {
	base = b;
}

/*******************************************************************************
* ForceTorqueFunc::SetIndex
* currently unused(?) function to set index if not set in constructor
*******************************************************************************/
void ForceTorqueFunc::SetIndex(int i) {
	index = i;
}

// =============================================================================

/*******************************************************************************
* LinRestorForce default constructor
* initializes array of ForceTorqueFunc objects and pointers to each force/torque
*******************************************************************************/
LinRestorForce::LinRestorForce() : forces{ {this, 0}, {this, 1}, {this, 2}, {this, 3}, {this, 4}, {this, 5} } {
	for (unsigned i = 0; i < 6; i++) {
		force_ptrs[i] = std::shared_ptr<ForceTorqueFunc>(forces + i, [](ForceTorqueFunc*) {});
		// sets force_ptrs[i] to point to forces[i] but since forces is on the stack, it is faster and it is 
		// automatically deallocated...shared pointers typically manage heap pointers, and will try deleting
		// them as soon as done. Doesn't work on stack array (can't delete stack arrays), we overload the 
		// default deletion logic to do nothing
		// Also! don't need to worry about deleting this later, because stack arrays are always deleted automatically
	}
}

/*******************************************************************************
* LinRestorForce constructor
* calls default constructor and initializes linear restoring stiffness force info
* from BodyFileInfo
* also initializes ChBody that this force will be applied to
*******************************************************************************/
LinRestorForce::LinRestorForce(BodyFileInfo& lin, std::shared_ptr<ChBody> object) : LinRestorForce() {
	bobber = object;
	fileInfo = lin;
	equil << fileInfo.get_equil_cg().eigen(), 0, 0, 0; // set equilibrium to (cg0, cg1, cg2, 0, 0, 0)
	prevTime = -1;
}

/*******************************************************************************
* LinRestorForce::matrixMult()
* calculates the matrix multiplication each time step for linear restoring stiffness
* f = [linear restoring stiffness matrix] [displacement vector]
*******************************************************************************/
ChVectorN<double, 6> LinRestorForce::matrixMult() { 
	if (bobber->GetChTime() == prevTime) {
		return currentForce;
	}
	prevTime = bobber->GetChTime();
	currentForce << bobber->GetPos().eigen(), bobber->GetRot().Q_to_Euler123().eigen();
	currentForce = equil - currentForce;
	currentForce = fileInfo.get_lin_matrix() * currentForce;
	return currentForce;
}

/*******************************************************************************
* LinRestorForce::coordinateFunc
* if index is in [0,6] the corresponding vector component of the force vector
* is returned
* otherwise a warning is printed and the force is interpreted to be 0
*******************************************************************************/
double LinRestorForce::coordinateFunc(int i) {
	if (i >= 0 && i < 6) {
		return matrixMult()[i];
	}
	else {
		std::cout << "wrong index" << std::endl;
		return 0;
	}
}

/*******************************************************************************
* LinRestorForce::SetForce
* used to initialize components of force (external ChForce pointer)
*******************************************************************************/
void LinRestorForce::SetForce(std::shared_ptr<ChForce> force) {
	force->SetF_x(force_ptrs[0]);
	force->SetF_y(force_ptrs[1]);
	force->SetF_z(force_ptrs[2]);
}

/*******************************************************************************
* LinRestorForce::SetTorque
* used to initialize components of torque (external ChForce pointer with TORQUE flag set)
*******************************************************************************/
void LinRestorForce::SetTorque(std::shared_ptr<ChForce> torque) {
	torque->SetF_x(force_ptrs[3]);
	torque->SetF_y(force_ptrs[4]);
	torque->SetF_z(force_ptrs[5]);
}

// =============================================================================

/*******************************************************************************
* BuoyancyForce constructor
* requires BodyFileInfo to initialize data
*******************************************************************************/
BuoyancyForce::BuoyancyForce(BodyFileInfo& file) {
	fileInfo = file;
	// get value from file
	//std::cout << "rho=" << fileInfo.get_rho() << " g=" << fileInfo.get_g() << " V=" << fileInfo.get_disp_vol() << std::endl;
	bf = fileInfo.get_rho() * fileInfo.get_g() * fileInfo.get_disp_vol();
	// set function to y = bf
	fc.Set_yconst(bf);
	// set pointer to function y=bf
	fc_ptr = std::shared_ptr<ChFunction_Const>(&fc, [](ChFunction_Const*) {} );
	// set force as function pointer in +z direction
	force.SetF_z(fc_ptr);
	// have force_ptr point to force
	force_ptr = std::shared_ptr<ChForce>(&force, [](ChForce*) {});
}

/*******************************************************************************
* BuoyancyForce::getForce_ptr()
* returns shared pointer to force vector for buoyancy force for use with Project
* Chrono
*******************************************************************************/
std::shared_ptr<ChForce> BuoyancyForce::getForce_ptr() {
	return force_ptr;
}

/*******************************************************************************
* IRF_func default constructor
* initializes object handling matrix math (base)
* and index for which DoF to use
*******************************************************************************/
IRF_func::IRF_func(ImpulseResponseForce* b, int i) : base(b), index(i) { }

/*******************************************************************************
* IRF_func::Clone()
* required overload function since IRF_func inherits from Chrono::ChFunction
*******************************************************************************/
IRF_func* IRF_func::Clone() const {
	return new IRF_func(*this);
}

/*******************************************************************************
* IRF_func::Get_y()
* overloaded function from Chrono::ChFunction to update force each timestep
*******************************************************************************/
double 	IRF_func::Get_y(double x) const {
	//std::cout << base->body->GetChTime() << "\t";
	return base->coordinateFunc(index);
}

/*******************************************************************************
* IRF_func::SetBase(ImpulseResponseForce* b)
* optional function for setting base object outside of constructor
*******************************************************************************/
void IRF_func::SetBase(ImpulseResponseForce* b) {
	base = b;
}

/*******************************************************************************
* IRF_func::SetIndex(int i)
* optional function for setting index outside of constructor
*******************************************************************************/
void IRF_func::SetIndex(int i) { 
	index = i;
}

/*******************************************************************************
* default constructor, only initializes forces and force pointers, use other
* constructor to fully initialize object
*******************************************************************************/
ImpulseResponseForce::ImpulseResponseForce() : forces{ {this, 0}, {this, 1}, {this, 2}, {this, 3}, {this, 4}, {this, 5} } {
	for (unsigned i = 0; i < 6; i++) {
		force_ptrs[i] = std::shared_ptr<IRF_func>(forces + i, [](IRF_func*) {});
		// sets force_ptrs[i] to point to forces[i] but since forces is on the stack, it is faster and it is 
		// automatically deallocated...shared pointers typically manage heap pointers, and will try deleting
		// them as soon as done. Doesn't work on stack array (can't delete stack arrays), we overload the 
		// default deletion logic to do nothing
		// Also! don't need to worry about deleting this later, because stack arrays are always deleted automatically
	}
}

/*******************************************************************************
* ImpulseResponseForce constructor, calls default constructor to initialize force 
* function/vectors. Then initializes several persistent variables
*******************************************************************************/
ImpulseResponseForce::ImpulseResponseForce(BodyFileInfo& file, std::shared_ptr<ChBody> object) : ImpulseResponseForce() {
	body = object;
	fileInfo = file;
	// initialize other things from file here
	velHistory.resize(file.get_K_dims(2));
	timeSteps = file.get_times();
	ChVectorN<double, 6> temp;
	for (int i = 0; i < 6; i++) {
		temp[i] = 0;
		currentForce[i] = 0;
	}
	for (int i = 0; i < 1001; i++) {		
		velHistory[i] = temp;
	}
	offset = 0;
	prevTime = -1;
}
/*******************************************************************************
* convolutionIntegral()
* currently works for 1 body, with no interpolation steps
*******************************************************************************/
ChVectorN<double, 6> ImpulseResponseForce::convolutionIntegral() {
	// since convolutionIntegral called for each DoF each timestep, we only want to 
	// calculate the vector force once each timestep. Save the prevTime
	if (body->GetChTime() == prevTime) {
		return currentForce;
	}
	prevTime = body->GetChTime();
	int size = 1001;
	// "shift" everything left 1
	offset--;
	if (offset < -1 * size) {
		offset += size;
	}
	int r = 6, c = 6;

	double* timeseries = new double [r*c*size]; // 1001x6x6
	double* tmp_s = new double [r*size]; // 1001x6
#define TIMESERIES(row,col,step) timeseries[(row)*c*size + (col)*size + (step)]
#define TMP_S(row,step) tmp_s[(row)*size + (step)]

	// set last entry as velocity
	for (int i = 0; i < 3; i++) { 
		velHistory[(((size + offset) % size) + size) % size][i] = body->GetPos_dt()[i];
		velHistory[(((size + offset) % size) + size) % size][i + 3] = body->GetRot_dt().Q_to_Euler123()[i]; 
	}
	int vi;
	for (int row = 0; row < 6; row++) {
		currentForce[row] = 0;
		for (int st = 0; st < size; st++) {
			vi = (((st + offset) % size) + size) % size; // vi takes care of circshift function from matLab
			TMP_S(row,st) = 0;
			for (int col = 0; col < 6; col++) {
				TIMESERIES(row,col,st) = fileInfo.get_impulse_resp(row, col, st)*velHistory[vi][col];
				TMP_S(row,st) += TIMESERIES(row,col,st);
			}
			if (st > 0) {
				currentForce[row] -= (TMP_S(row,st - 1) + TMP_S(row, st)) / 2.0 * (timeSteps[st] - timeSteps[st - 1ull]);
			}
		}
	}
	// Deallocate memory
#undef TIMESERIES
#undef TMP_S
    delete[] timeseries;
	delete[] tmp_s;
	return currentForce;
}

/*******************************************************************************
* ImpulseResponseForce::coordinateFunc(int i)
* returns the ith component of impulse response force for i = 0,...,5
*******************************************************************************/
double ImpulseResponseForce::coordinateFunc(int i) {
	if (i >= 0 && i < 6) {
		return convolutionIntegral()[i];
	}
	else {
		std::cout << "wrong index" << std::endl;
		return 0;
	}
}

/*******************************************************************************
* ImpulseResponseForce::SetForce()
* initializes external pointer force with ChFunction components
*******************************************************************************/
void ImpulseResponseForce::SetForce(std::shared_ptr<ChForce> force) {
	force->SetF_x(force_ptrs[0]);
	force->SetF_y(force_ptrs[1]);
	force->SetF_z(force_ptrs[2]);
}

/*******************************************************************************
* ImpulseResponseForce::SetTorque()
* initializes external pointer torque with ChFunction components
*******************************************************************************/
void ImpulseResponseForce::SetTorque(std::shared_ptr<ChForce> torque) {
	torque->SetF_x(force_ptrs[3]);
	torque->SetF_y(force_ptrs[4]);
	torque->SetF_z(force_ptrs[5]);
}

// -----------------------------------------------------------------------------
// ChLoadAddedMass
// -----------------------------------------------------------------------------
ChLoadAddedMass::ChLoadAddedMass(std::shared_ptr<ChBody> body,  ///< object to apply additional inertia to
	const BodyFileInfo& file) : ChLoadCustom(body) {
	inf_freq = file.get_added_mass_matrix();
}
//ChLoadAddedMass::ChLoadAddedMass(std::shared_ptr<ChBody> body,  ///< object to apply additional inertia to
//	const ChVector<>& m_offset,      ///< offset of the center of mass, in body coordinate system
//	const double m_mass)             ///< added mass [kg]
//	: ChLoadCustom(body), c_m(m_offset), mass(m_mass)
//{
//	//this->SetInertiaXX(m_IXX);
//	//this->SetInertiaXY(m_IXY);
//}

// The inertia tensor functions

//void ChLoadAddedMass::SetInertia(const ChMatrix33<>& newXInertia) {
//	I = newXInertia;
//}

//void ChLoadAddedMass::SetInertiaXX(const ChVector<>& iner) {
//	I(0, 0) = iner.x();
//	I(1, 1) = iner.y();
//	I(2, 2) = iner.z();
//}

//void ChLoadAddedMass::SetInertiaXY(const ChVector<>& iner) {
//	I(0, 1) = iner.x();
//	I(0, 2) = iner.y();
//	I(1, 2) = iner.z();
//	I(1, 0) = iner.x();
//	I(2, 0) = iner.y();
//	I(2, 1) = iner.z();
//}

//ChVector<> ChLoadAddedMass::GetInertiaXX() const {
//	ChVector<> iner;
//	iner.x() = I(0, 0);
//	iner.y() = I(1, 1);
//	iner.z() = I(2, 2);
//	return iner;
//}

//ChVector<> ChLoadAddedMass::GetInertiaXY() const {
//	ChVector<> iner;
//	iner.x() = I(0, 1);
//	iner.y() = I(0, 2);
//	iner.z() = I(1, 2);
//	return iner;
//}

void ChLoadAddedMass::ComputeQ(ChState* state_x, ChStateDelta* state_w) { } // state_x is position, state_w is velocity

void ChLoadAddedMass::ComputeJacobian(ChState* state_x,       ///< state position to evaluate jacobians
	ChStateDelta* state_w,  ///< state speed to evaluate jacobians
	ChMatrixRef mK,         ///< result dQ/dx
	ChMatrixRef mR,         ///< result dQ/dv
	ChMatrixRef mM          ///< result dQ/da
) {
	// fetch speeds as 3d vectors for convenience
	//ChVector<> v_x = state_w->segment(0, 3); // abs. 
	//ChVector<> v_w = state_w->segment(3, 3); // local 
	// (note: accelerations should be fetched from a "state_acc" like we did for speeds with state_w, 
	// but acc. are not available in ComputeQ inputs... maybe in future we should change the ChLoad 
	// class to support also this. For this special case, it works also with the following trick, but 
	// would fail the default numerical differentiation in ComputeJacobian() for the M=dQ/da matrix, so 
	// we override ComputeJacobian and provide the analytical jacobians)
	//auto mbody = std::dynamic_pointer_cast<ChBody>(this->loadable);
	//ChVector<> a_x = mbody->GetA().transpose() * mbody->GetPos_dtdt(); // local 
	//ChVector<> a_w = mbody->GetWacc_loc(); // local 

	//ChStarMatrix33<> wtilde(v_w);  // [w~]
	//ChStarMatrix33<> ctilde(c_m);  // [c~]

	// Analytic expression of inertial load jacobians.
	// Note signs: positive as they go in LHS. 

	//set mass matrix here
	jacobians->M = inf_freq;
	//jacobians->M.setZero();
	//jacobians->M(0, 0) = 71.57346 * 1000;
	//jacobians->M(1, 1) = 71.57342 * 1000;
	//jacobians->M(2, 2) = 130.8768 * 1000;
	//jacobians->M(3, 3) = 286.2663 * 1000;
	//jacobians->M(4, 4) = 286.2663 * 1000;
	//jacobians->M(5, 5) = 6.817555E-8 * 1000;
	//jacobians->M(3, 1) = -143.14 * 1000;
	//jacobians->M(4, 0) = 143.1401 * 1000;
	//jacobians->M(1, 3) = -143.1401 * 1000;
	//jacobians->M(0, 4) = 143.1402 * 1000;

	// R gyroscopic damping matrix terms (6x6, split in 3x3 blocks for convenience)
	// 0 for added mass
	jacobians->R.setZero();

	// K inertial stiffness matrix terms (6x6, split in 3x3 blocks for convenience)
	// 0 for added mass
	jacobians->K.setZero();
}

// The default base implementation in ChLoadCustom could suffice, but here reimplement it in sake of higher speed
// because we can exploiti the sparsity of the formulas.
void ChLoadAddedMass::LoadIntLoadResidual_Mv(ChVectorDynamic<>& R, const ChVectorDynamic<>& w, const double c) {
	if (!this->jacobians)
		return;

	if (!loadable->IsSubBlockActive(0))
		return;

	// fetch w as a contiguous vector
	//ChVector<> a_x = w.segment(loadable->GetSubBlockOffset(0), 3);
	//ChVector<> a_w = w.segment(loadable->GetSubBlockOffset(0) + 3, 3);

	// R+=c*M*a  
	// segment gives the chunk of vector starting at the first argument, and going for as many elements as the second argument...
	// in this case, segment gets the 3vector starting at the 0th DOF's offset (ie 0)
	//R.segment(loadable->GetSubBlockOffset(0), 3) += c * (this->mass * (a_x + chrono::Vcross(a_w, this->c_m))).eigen();
	// in this case, segment gets the 3vector starting at the 0th DOF's + 3 offset (ie 3)
	//R.segment(loadable->GetSubBlockOffset(0) + 3, 3) += c * (this->mass * chrono::Vcross(this->c_m, a_x) + this->I * a_w).eigen();
	// since R is a vector, we can probably just do R += C*M*a with no need to separate w into a_x and a_w above
	R += c * jacobians->M * w;
	//std::cout << jacobians->M << std::endl << std::endl;
	// this still works, so that's neat
}