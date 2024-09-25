\page calibration REDAC calibration process

The REDAC computer uses a digitally steered calibration process to reduce offset and gain errors of the involved stages in the analog compute elements. Calibration can be thought of as "analog computing error correction" which only scales thanks to the automatization possible with the digital hybrid controller. Continously applied error correction can reduce long term shifts such as temperature drifts or aging as well as constant noise levels. In particular, offset and gain errors can be understood as the first parts of a tailor approximation of unwanted non-linearities such as hysteresis.

This way, the exactness and bandwith can be significantly improved (at the order of 10% of relative exactness). In order to make calibration possible in the first place, many additional hardware parts have been added. Example hardware additions are:

- the *SH block* (short for *sample and hold block*)
- MDACs on the math blocks
- particular reconfigurability of the U-block in order to feed in well-known voltages

Calibration happens completely within the firmware and transparently to the end user. It happens at startup and before user-submitted *runs* are starting.

This document shall give a short overview about the calibration scheme. More documentation is available at other places and shall be evventually merged here.

## General idea of calibrating the U-C-I path

The U, C and I blocks are the heart of the interconnection system. The circuit path throught this system, refered here to as *U-C-I path*, is, amongst others, responsible for scaling up and adding together signals. Because every signal has to pass through the U-C-I path at least once, reducing error here is a necessity for achieving accurate calculations. Therefore, the U-C-I path gets freshly calibrated before every run.

Mathematically, every output of the U-C-I path represents a linear combination of the inputs. Consequently, the errors that occur in this path are constant gain and offset errors, that get corrected by a three-phase calibration algorithm:

1. All input signals are switched to zero. The SH-block which is responsible for correcting offset errors gets activated and thus automatically removes any signal offset.
2. Every C-block coefficient gets turned to zero. Then all input signals are switched to one. Now one C-block coefficient gets set to one, letting through one signal into the I-block. The now measured result, the gain error on this specific signal path. The altered C-block coefficient now gets a gain correction coefficient which is equal to the inverse measured gain error.
This process is repeated for every single signal used in the calculation.
3. All input signals are switched to zero again. The SH-block gets activated a second time to remove any offset errors that are caused by the altered C-block coefficients and corrections.

After this, the U-C-I path is fully calibrated and signals can be added and scaled with an almost negligible error.
