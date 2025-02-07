class PID {
  public:
    /* ... */

    /// Update the controller with the given position measurement `meas_y` and
    /// return the new control signal.
    float update(float reference, float meas_y);

  private:
    float kp, ki, kd, alpha, Ts;
    float max_output;
    float min_output;
    float integral;
    float old_ef;
};