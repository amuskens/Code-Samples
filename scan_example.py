"""
MARIUS measurement system.
This function is part of a larger class (Session). It has been extracted here to show an example of my Python code. 

(C) 2015 Anders Muskens

"""

import numpy as np # among others... 

# This function initiates a fibre optic cable sweep in a grid pattern attempting to align the input and output fibres to the locations where the coupled signal power is the greatest. 

# The setup includes a variable frequency and power laser, as well as an optical spectrum analyzer 
# which measures the wideband power performance. The data can be interpolated using a suitable interpolation algorithm 
# (cubic, bicubic, nearest neighbor, etc.) to attempt to guess where the maxima might be. 
# In typical operation, the sweep would be done multiple times with increasingly smaller grid sizes centered around
# the interpolated maxima location. This would allow the system to algin the fibre optic cable very precisely. 

# parameters
# self
# traget: either the input or output fibre optic cable. 
# position: position to start in (nm) as a tuple (x,y,z)
# grid_width, grid_height: Size (in nm) of the sweep grid.
# grid_xpoints, grid_ypoints: number of samples to take in each dimension of the grid. 
# passes: number of passes to average from the spectrum analyzer. 
# interpolation_type: type of interpolation to use
# update_callback: callback to execute upon the completion of a pass. Used to update the UI graphs
# complete_callback: callback to execute upon the completion of the scan.
# power, wavelength: Power (mW) and wavelength (nm) of the laser
# grid_width, grid_height: the dimensions (in nanometers) of the initial sweep grid.

def findMaxima(self,target,position,
         power,wavelength,
         grid_width,grid_height,
         grid_xpoints,grid_ypoints,
         passes,
         interpolation_type,
         update_callback,
         complete_callback):
      
	# abort if a systemwide abort is in operation.
	if self.stopping.get(): 
	return False      

	# set up laser for the sweep
	self.deactivateLaser()
	self.setLaserParameters(wavelength,power)
	self.activateLaser()
	sleep(1.0)    # wait for the laser to be ready.   

	# setup the power meter for measurement.
	self.meter_lock.acquire()
	self.meter.setup(wavelength)
	self.meter_lock.release()

	# setup the initial scan grid. 
	self.grid_width = grid_width
	self.grid_height = grid_height

	# start search pattern for input
	# -------------------------------
	# create an empty linear space to put the data in. Fill it with NaN values.
	xspace = np.linspace(0,grid_width,num = grid_xpoints,endpoint=False)
	yspace = np.linspace(0,grid_height,num = grid_ypoints,endpoint=False)
	x, y = np.meshgrid(xspace,yspace,sparse=False, indexing='ij')
	I = x * 0
	I = np.empty((grid_xpoints,grid_ypoints))
	I[:] = np.NAN

	nx = grid_xpoints
	ny = grid_ypoints

	# get current positions of teh target fibre cable
	if target == "input":
	current = self.stage.getInputPosition()
	elif target == "output":
	current = self.stage.getOutputPosition()

	# determine the start point from the format given by the input. 
	# If a z coordinate is not given, start the scan at the current z poisiotn. 
	if len(position) == 3:
		start_point = (position[0] - grid_width * 0.5,
				position[1] - grid_height * 0.5,
				position[2])
	elif len(position) == 2:
		start_point = (position[0] - grid_width * 0.5,
				position[1] - grid_height * 0.5,
				current[2])
	else:  
		start_point = current

	# Measure
	# -------
	if target == "input":
		status = self.stage.moveInputToPoint(start_point)
	elif target == "output":
		status = self.stage.moveOutputToPoint(start_point)    

	if status == False and test == False: return False	# failure to move fibres.

	j = 0  # index to determine if we are in an odd row
	total = nx * ny	# total number of samples in grid

	points = []
	values = []

	for iy in range(ny):
		for ix in range(nx):

			# For determining progress for the progress bar. 
			px = ix + 1
			py = iy + 1
			progress = float(px + (py - 1) * nx) / total

			if (not j % 2 == 0): # j is odd
			   ix = nx - 1 - ix
			   
			pt = add((x[ix,iy],y[ix,iy],0),(start_point))  
			if target == "input":
			   status = self.stage.moveInputToPoint(pt)
			   position = self.stage.getInputPosition()
			elif target == "output":
			   status = self.stage.moveOutputToPoint(pt)
			   position = self.stage.getOutputPosition()
			   
			if status == False and test == False: return False	 # abort on a movement failure

			# take measurement (wait a little bit for vibration to cease.)
			sleep(self.operation_delay)
			mp = self.meterGetPower(passes)

			I[ix,iy] = mp
			values.append(mp)
			data = { "x": x, "y": y, "I": I }

			update_callback(progress,data)	# run the update callback
		j+=1

	# scan is complete
	self.deactivateLaser()

	# find the maximum by parsing the array
	maximum = -np.inf
	maximum_location = (0,0)

	for ix in range(nx):
		for iy	 in range(ny):
		  if I[ix,iy] > maximum:
			maximum_location = (ix,iy)
			maximum = I[ix,iy]
	  
	measured_maximum = maximum
	abs_maximum_location  = add((x[maximum_location],y[maximum_location],0),start_point)   

	# interpolate
	# -----------
	interpolate_depth = INTERPOLATION_DEPTH
	xspace_new = np.linspace(0,grid_width,num = interpolate_depth,endpoint=True)
	yspace_new = np.linspace(0,grid_height,num = interpolate_depth,endpoint=True)
	x_new, y_new = np.meshgrid(xspace_new,yspace_new,sparse=False, indexing='ij')
	nx_new = interpolate_depth
	ny_new = interpolate_depth 
	points = np.array(points)
	values = np.array(values)

	# Try to interpolate using the algorithm. If there is a failure, then return a null interpolation array. 
	try:
		if interpolation_type == "Bivariate Spline":
			tck = interpolate.bisplrep(x, y, I,s=0)
			I_new = interpolate.bisplev(x_new[:,0], y_new[0,:], tck)
		elif interpolation_type == "Linear":
			f = interpolate.interp2d(xspace, yspace, I, kind='linear')
			I_new = f(xspace_new,yspace_new)
		elif interpolation_type == "Cubic":
			f = interpolate.interp2d(xspace, yspace, I, kind='cubic')
			I_new = f(xspace_new,yspace_new)
		elif interpolation_type == "Quintic":
			f = interpolate.interp2d(xspace, yspace, I, kind='quintic')
			I_new = f(xspace_new,yspace_new)
		else:
			I_new = None 
	except:
		I_new = None
		# run the completion callback to update the UI. 
		interpolated_maximum_location = ""
		complete_callback(measured_maximum,abs_maximum_location,interpolated_maximum_location,
						 x,y,I,x_new,y_new,I_new,True)
		return True


	# find the interpolated maximum by parsing the array
	interpolated_maximum = -np.inf
	maximum_location = (0,0)

	for ix in range(nx_new - 1):
		for iy in range(ny_new - 1):
		  if I_new[ix,iy] > interpolated_maximum:
			 maximum_location = (ix,iy)
			 interpolated_maximum = I_new[ix,iy]
			  

	interpolated_maximum_location  = add((x_new[maximum_location],y_new[maximum_location],0),start_point)

	# update the UI
	complete_callback(measured_maximum,abs_maximum_location,interpolated_maximum_location,
					x,y,I,x_new,y_new,I_new,True)

	return True
   