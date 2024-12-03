import serial
import threading
import customtkinter as ctk
import time

unit_dict = {
    "Turbidity": "NTU",
    "Microplastic": "particles/L",
    "DoLevel": "mg/L"
}


def categorize_value(value):
    """
    Categorizes the value based on the number of digits before the decimal point.
    """
    if '.' in value:
        before_decimal = value.split('.')[0]
        if len(before_decimal) == 1:
            return "DoLevel"
        elif len(before_decimal) == 2:
            return "Turbidity"
    else:
        return "Microplastic"


class SerialApp:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 Serial Communication")
        ctk.set_appearance_mode("Dark")  # Set dark mode
        ctk.set_default_color_theme("blue")  # Set the color theme

        # Serial connection attributes
        self.ser = None
        self.serial_thread = None
        self.stop_event = threading.Event()
        self.lock = threading.Lock()

        # Create the UI
        self.create_widgets()

    def create_widgets(self):
        # COM Port Entry
        self.com_label = ctk.CTkLabel(self.root, text="COM Port:")
        self.com_label.grid(row=0, column=0, padx=10, pady=10, sticky="e")

        self.com_port_entry = ctk.CTkEntry(self.root, placeholder_text="Enter COM Port (e.g., COM7)")
        self.com_port_entry.grid(row=0, column=1, padx=10, pady=10)
        self.com_port_entry.insert(0, "COM7")

        # Command Entry
        self.command_label = ctk.CTkLabel(self.root, text="Command:")
        self.command_label.grid(row=1, column=0, padx=10, pady=10, sticky="e")

        self.command_entry = ctk.CTkEntry(self.root, placeholder_text="Enter Command (START, RESET)")
        self.command_entry.grid(row=1, column=1, padx=10, pady=10)

        # Buttons
        self.connect_button = ctk.CTkButton(self.root, text="Connect", command=self.connect_serial)
        self.connect_button.grid(row=0, column=2, padx=10, pady=10)

        self.disconnect_button = ctk.CTkButton(self.root, text="Disconnect", command=self.disconnect_serial,
                                               state="disabled")
        self.disconnect_button.grid(row=0, column=3, padx=10, pady=10)

        self.send_button = ctk.CTkButton(self.root, text="Send", command=self.send_command, state="disabled")
        self.send_button.grid(row=1, column=2, columnspan=2, padx=10, pady=10)

        # Textbox for responses
        self.response_label = ctk.CTkLabel(self.root, text="STM32 Responses:")
        self.response_label.grid(row=2, column=0, columnspan=4, padx=10, pady=10, sticky="w")

        self.response_textbox = ctk.CTkTextbox(self.root, width=500, height=200, state="disabled")
        self.response_textbox.grid(row=3, column=0, columnspan=4, padx=10, pady=10)

    def connect_serial(self):
        # Get COM port from entry
        com_port = self.com_port_entry.get().strip()
        baud_rate = 115200  # Default baud rate

        try:
            # Initialize the serial connection
            self.ser = serial.Serial(com_port, baud_rate, timeout=1)
            self.log_to_text(f"Connected to {com_port} at {baud_rate} baud.")

            # Start the serial reading thread
            self.stop_event.clear()
            self.serial_thread = threading.Thread(target=self.read_from_serial, daemon=True)
            self.serial_thread.start()

            # Update button states
            self.connect_button.configure(state="disabled")
            self.disconnect_button.configure(state="normal")
            self.send_button.configure(state="normal")
        except serial.SerialException as e:
            self.log_to_text(f"Error: {e}")

    def disconnect_serial(self):
        # Stop the reading thread
        self.stop_event.set()
        if self.serial_thread:
            self.serial_thread.join()

        # Close the serial connection
        if self.ser and self.ser.is_open:
            self.ser.close()
            self.log_to_text("Serial connection closed.")

        # Update button states
        self.connect_button.configure(state="normal")
        self.disconnect_button.configure(state="disabled")
        self.send_button.configure(state="disabled")

    def send_command(self):
        # Get command from entry
        command = self.command_entry.get().strip().upper()
        if not command:
            self.log_to_text("Error: Command cannot be empty.")
            return

        if command not in ["START", "RESET", "EXIT"]:
            self.log_to_text("Invalid command. Use 'START', 'RESET', or 'EXIT'.")
            return

        with self.lock:
            try:
                self.ser.write((command + "\n").encode())
                self.log_to_text(f"Sent: {command}")
            except Exception as e:
                self.log_to_text(f"Error: {e}")

    def read_from_serial(self):
        """
        Continuously read data from the serial port and update the GUI.
        """
        while not self.stop_event.is_set():
            with self.lock:
                if self.ser and self.ser.in_waiting > 0:
                    try:
                        response = self.ser.read(self.ser.in_waiting).decode().strip()
                        if response:
                            label = categorize_value(response)
                            self.log_to_text(f"{label} Data: {response} {unit_dict[label]}")
                    except Exception as e:
                        self.log_to_text(f"Error reading from serial: {e}")
            time.sleep(0.08)  # Small delay to avoid high CPU usage

    def log_to_text(self, message):
        """
        Log a message to the text box (thread-safe).
        """
        self.response_textbox.configure(state="normal")
        self.response_textbox.insert("end", f"{message}\n")
        self.response_textbox.yview("end")  # Auto-scroll
        self.response_textbox.configure(state="disabled")


# Run the CustomTkinter app
if __name__ == "__main__":
    ctk.set_appearance_mode("Dark")
    root = ctk.CTk()
    app = SerialApp(root)
    root.mainloop()
