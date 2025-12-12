"""
Integration tests for FSL using Python sockets.
Simulates GSL (UDP client) and two apps (UDS clients/servers).
"""

import socket
import os
import time
import struct
import threading


def send_udp_to_fcom(opcode, payload, udp_ip, udp_port):
    """Simulate GSL: Send UDP packet to FSL with header and payload."""
    # Example header: opcode (uint16), length (uint16), id (uint32)
    msg_seq_id = 1
    header = struct.pack("<HHI", opcode, len(payload), msg_seq_id)
    packet = header + payload.encode()
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.sendto(packet, (udp_ip, udp_port))
    s.close()


def send_uds_to_fcom(uds_path, payload):
    """Simulate app: Send payload to FSL UDS server socket."""
    # Ensure parent directory exists if SENSOR_INSTANCE is set
    instance = os.environ.get("SENSOR_INSTANCE")
    if instance:
        parent = os.path.dirname(uds_path)
        if not os.path.exists(parent):
            os.makedirs(parent, exist_ok=True)
    s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    s.sendto(payload.encode(), uds_path)
    s.close()


def receive_uds(uds_path, timeout=2):
    """Simulate app: Receive from FSL UDS client socket."""
    # Ensure parent directory exists if SENSOR_INSTANCE is set
    instance = os.environ.get("SENSOR_INSTANCE")
    if instance:
        parent = os.path.dirname(uds_path)
        if not os.path.exists(parent):
            os.makedirs(parent, exist_ok=True)
    if os.path.exists(uds_path):
        os.unlink(uds_path)
    s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    s.bind(uds_path)
    s.settimeout(timeout)
    try:
        data, _ = s.recvfrom(4096)
        return data
    except socket.timeout:
        return None
    finally:
        s.close()
        os.unlink(uds_path)


def uds_receiver_thread(uds_path, result_container):
    data = receive_uds(uds_path)
    result_container.append(data)


def test_udp_to_uds():
    """Test: GSL sends UDP, FSL routes to correct UDS client."""
    fsl_udp_ip = "127.0.0.1"
    fsl_udp_port = 9910
    instance = int(os.environ.get("SENSOR_INSTANCE", "0"))
    prefix = f"/tmp/sensor{instance}/" if instance > 0 else "/tmp/"
    app_uds_clients = [
        prefix + "UL_APP1",
        prefix + "UL_APP2",
        prefix + "UL_APP3",
        # Add more uplink UDS client paths as needed
    ]
    for i, uds_client_path in enumerate(app_uds_clients, start=1):
        opcode = i  # Use different opcode for each app if needed
        payload = f"hello_from_gsl_{i}"
        result = []
        t = threading.Thread(target=uds_receiver_thread, args=(uds_client_path, result))
        t.start()
        time.sleep(0.5)  # Give receiver time to bind
        print(f"Uplink: sending UDP to FSL for {uds_client_path}...")
        send_udp_to_fcom(opcode, payload, fsl_udp_ip, fsl_udp_port)
        t.join()
        received = result[0] if result else None
        assert (
            received is not None
        ), f"No data received on UDS client socket {uds_client_path}"
        print(f"Uplink: received on {uds_client_path}:", received)


def test_uds_to_udp():
    """Test: App sends to UDS server, FSL routes to UDP (GSL)."""
    gcom_udp_ip = "127.0.0.1"
    gcom_udp_port = 9010
    instance = int(os.environ.get("SENSOR_INSTANCE", "0"))
    prefix = f"/tmp/sensor{instance}/" if instance > 0 else "/tmp/"
    app_uds_paths = [
        prefix + "DL_APP1_H",
        prefix + "DL_APP2_H",
        prefix + "DL_APP3_H",
        # Add more app UDS paths as needed
    ]
    for i, uds_server_path in enumerate(app_uds_paths, start=1):
        payload = f"hello from app{i}"
        # Start UDP receiver (GSL)
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind((gcom_udp_ip, gcom_udp_port))
        s.settimeout(2)
        print(f"Downlink: sending message to UDS server {uds_server_path}...")
        send_uds_to_fcom(uds_server_path, payload)
        print("Downlink: receiving message from UDP...")
        try:
            data, addr = s.recvfrom(4096)
            print("Downlink: received message from UDP:", data)
            assert data is not None, f"No UDP data received for {uds_server_path}"
        except socket.timeout:
            print(f"No UDP data received for {uds_server_path}")
            assert False, f"No UDP data received for {uds_server_path}"
        finally:
            s.close()


def main():
    print("--- Integration Test: UDP to UDS ---")
    test_udp_to_uds()
    print("--- Integration Test: UDS to UDP ---")
    test_uds_to_udp()
    print("All integration tests passed.")


if __name__ == "__main__":
    main()
