import hashlib


def parse_measurements(path):
    """
    Parse measurements.txt and convert it to structured data.
    Each entry in the output contains:
      - id1: Identifier (e.g., STA 00)
      - token: None (default, as token is not in the file)
      - counter: Simulated or assigned counter value
      - result: Timing value from the file
    """
    structured_data = []
    with open(path, "r") as f:
        for line in f:
            if line.startswith("STA"):
                # Extract STA identifier and timing value
                parts = line.split(":")
                id1 = parts[0].strip()  # STA XX
                timing_value = int(parts[1].strip())  # Timing value

                # Append to structured data
                structured_data.append(
                    (id1, None, 1, timing_value)
                )  # Default counter=1
    return structured_data


def simulate(pw, id1, id2):
    """
    Simulate the Dragonfly handshake's hash-to-curve behavior to produce timing results.
    This example uses a simplified hash-based simulation.

    Args:
    - pw: Password to test.
    - token: Optional token (None if not applicable).
    - id1: Spoofed identity (e.g., MAC address of attacker).
    - id2: Victim's identity (e.g., MAC address of AP).
    - counter: Iteration counter for the test.

    Returns:
    - Simulated timing value or element test result.
    """
    # Create a unique input based on the parameters
    input_data = f"{pw}-{id1}-{id2}"

    # Hash the input data
    hashed = hashlib.sha256(input_data.encode()).hexdigest()

    # Simulate timing or test result
    # Example: Convert the hash into a timing value scaled to match observed ranges
    simulated_time = (
        sum(bytearray(hashed.encode())) % 20000
    )  # Scale to match measurement ranges

    return simulated_time


if __name__ == "__main__":
    file_path = "measurements.txt"
    data = parse_measurements(file_path)

    for t in data:
        if t[0] == "STA 01":
            print(t)

    # Example inputs
    password = "abcdefgh"
    token = None
    id1 = "00:33:00:00:00:01"
    id2 = "00:11:00:00:00:00"
    counter = 1

    # Call the simulate function
    simulated_result = simulate(password, token, id1, id2, counter)
    print(f"Simulated result: {simulated_result}")
