\page calibration REDAC calibration process

Before the REDAC/LUCIDAC can start its calculations, it has to undergo several calibration procedures to ensure, that the upcoming calculations are delivered with the highest amount of precision.

## General idea of calibrating the U-C-I path

The U-C-I path is responsible for scaling up and adding together signals. Because every signal has to pass through the U-C-I path at least once, reducing error here is a necessity for achieving accurate calculations. Therefore, the U-C-I path gets freshly calibrated before every run.

Mathematically, every output of the U-C-I path represents a linear combination of the inputs. Consequently, the errors that occur in this path are constant gain and offset errors, that get corrected by a three-phase calibration algorithm:

1. All input signals are switched to zero. The SH-block which is responsible for correcting offset errors gets activated and thus automatically removes any signal offset.
2. Every C-block coefficient gets turned to zero. Then all input signals are switched to one. Now one C-block coefficient gets set to one, letting through one signal into the I-block. The now measured result, the gain error on this specific signal path. The altered C-block coefficient now gets a gain correction coefficient which is equal to the inverse measured gain error.
This process is repeated for every single signal used in the calculation.
3. All input signals are switched to zero again. The SH-block gets activated a second time to remove any offset errors that are caused by the altered C-block coefficients and corrections.

After this, the U-C-I path is fully calibrated and signals can be added and scaled with an almost negligible error.
