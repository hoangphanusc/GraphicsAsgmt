Assignement 2: Roller Coaster
Program features:
* Required criteria:
Complete all levels.
	o Properly render Catmull-Rom splines to represent the track.
	o Render a texture-mapped ground and sky.
	o Render a rail cross-section.
	o Move the camera at a reasonable speed in a continuous path and orientation along the coaster.
	o Render the coaster in an interesting manner (good visibility, realism).
	o Run at interactive frame rates (>15fps at 640x480)
	x JPEG frames: Please take a look at the provided video (Capture.flv)
	
* Extras
	o Render double rail (like in real railroad tracks).
	o Draw additional scene elements: crossbars & column
	o Modify the velocity with which the camera moves to make it physically realistic in terms of gravity.
	o Derive the steps that lead to the physically realistic equation of updating the u: see below

According to conservation of energy law, assuming no loss of energy due to air friction etc, the sum of the
potential energy (mgh) and kinetic energy (1/2*mv^2) of the coaster must remain constant through out the ride.
At the highest point of the coaster, Hmax, potential energy is max. Assume velocity here = 0.
Then at all point during the ride, 1/2 mv^2 + mg Hcurr= mg * Hmax, or v = sqrt(2g*deltaH)
	This is the speed of the coaster at height Hcurr.
To get the change in our location, we multiply this speed with delta time:
	deltaP = deltaT * v = deltaT * sqrt(2g*deltaH)
To find the change in curve parameter u, we need to know how much u changes with a change in position.
We can get the opposite from the tangent, since it's the derivative of p with respect to u (d/du).
Evaluating the magnitude of the tangent with Ucurr gives the change in p according to u at our location.
Reverse this to find the change in u according to P (du/dp = 1/(dp/du))
So to find the new u:
	u_new   = u_old + change in u
			= u_old + du/dp * deltaP
			= u_old + du/dp * deltaT * sqrt(2g*deltaH)
			= u_old + deltaT * sqrt(2g*deltaH) / (dp/du)