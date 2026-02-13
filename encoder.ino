void EncoderInit() {
  Serial.println("ENCODER INIT");
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);
  // pinMode(buttonPin, INPUT_PULLUP);

  digitalWrite(encoderPinA, HIGH); // turn pullup resistor on
  digitalWrite(encoderPinB, HIGH); // turn pullup resistor on
}
// FSM State Table
// Index: (OldState << 2) | NewState
// Value: 1 (Valid Transition), 0 (Invalid/No Change)
// This table treats both CW and CCW rotations as incrementing the counter.
const int8_t encoder_states[] = {0, 1, 1, 0, 1, 0, 0, 1,
                                 1, 0, 0, 1, 0, 1, 1, 0};

void updateEncoder() {
  logoutTimerCnt = 0;
  gameTimerCnt = 0;

  int MSB = digitalRead(encoderPinA); // MSB = most significant bit
  int LSB = digitalRead(encoderPinB); // LSB = least significant bit

  int encoded = (MSB << 1) | LSB; // converting the 2 pin value to single number
  int stateIdx = (lastEncoded << 2) | encoded; // Combine old and new state

  // Update encoderValue based on state table
  if (encoder_states[stateIdx]) {
    gameFSM.notifyLeverStepFromISR();
  }

  lastEncoded = encoded; // store this value for next time
}
