import argparse
import os
from datetime import datetime

# Placeholder for databento import.
# If you have the databento library installed, uncomment the following line:
import databento as db
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def visualize_order_book(dbn_filepath, output_prefix="order_book_visualization"):
    """
    Visualizes the order book from a Databento DBN file.

    Args:
        dbn_filepath (str): Path to the Databento DBN file.
        output_prefix (str): Prefix for the output SVG and PNG filenames.
    """
    print(f"[{datetime.now()}] Processing DBN file: {dbn_filepath}")

    # --- 1. Read DBN file ---
    # This part assumes the databento library is installed and can read DBN files.
    # If you don't have it, you might need to install it: pip install databento
    # The exact API might vary. This is a common pattern for data libraries.
    try:
        data = pd.DataFrame()  # Initialize as empty DataFrame

        if os.path.splitext(dbn_filepath)[1].lower() == ".dbn" and "db" in globals():
            print(
                f"[{datetime.now()}] Attempting to use databento library to read DBN file: {dbn_filepath}"
            )
            try:
                read_start_time = datetime.now()
                raw_data = db.DBNStore.from_file(dbn_filepath).to_df()
                read_end_time = datetime.now()
                print(f"[{datetime.now()}] DBN file read took: {read_end_time - read_start_time}")

                if raw_data.empty:
                    print(
                        f"[{datetime.now()}] Error: DBN file '{dbn_filepath}' read successfully but contains no data. Exiting."
                    )
                    return  # Exit if no data
                else:
                    print(f"[{datetime.now()}] Successfully read {len(raw_data)} records from DBN file.")
                    data = (
                        raw_data.copy()
                    )  # Work on a copy to avoid SettingWithCopyWarning

                    # Filter for MBO messages. Assuming rtype 160 is MBO based on debug output.
                    MBO_RTYPE_CODE = 160  # Corrected based on debug output

                    filter_start_time = datetime.now()
                    initial_len = len(data)
                    if (
                        "rtype" in data.columns
                        and MBO_RTYPE_CODE in data["rtype"].unique()
                    ):
                        data = data[data["rtype"] == MBO_RTYPE_CODE]
                        if data.empty and initial_len > 0:
                            print(
                                f"[{datetime.now()}] Error: Filtering by 'rtype'={MBO_RTYPE_CODE} (MBO) resulted in an empty dataset for '{dbn_filepath}'. Exiting."
                            )
                            return  # Exit if no data after filtering
                    elif "schema" in data.columns and "mbo" in data["schema"].unique():
                        data = data[data["schema"] == "mbo"]
                        if data.empty and initial_len > 0:
                            print(
                                f"[{datetime.now()}] Error: Filtering by 'schema'='mbo' resulted in an empty dataset for '{dbn_filepath}'. Exiting."
                            )
                            return  # Exit if no data after filtering
                    else:
                        print(
                            f"[{datetime.now()}] Error: No 'rtype' column with MBO code ({MBO_RTYPE_CODE}) or 'schema'='mbo' found in '{dbn_filepath}'. Exiting."
                        )
                        return  # Exit if not MBO data

                    # Filter by instrument_id if present and multiple instruments exist
                    if (
                        "instrument_id" in data.columns
                        and data["instrument_id"].nunique() > 1
                    ):
                        first_instrument_id = data["instrument_id"].iloc[0]
                        print(
                            f"[{datetime.now()}] Multiple instruments found. Visualizing data for the first instrument: {first_instrument_id}"
                        )
                        data = data[data["instrument_id"] == first_instrument_id]
                        if data.empty and initial_len > 0:
                            print(
                                f"[{datetime.now()}] Error: Filtering by instrument_id '{first_instrument_id}' resulted in an empty dataset for '{dbn_filepath}'. Exiting."
                            )
                            return  # Exit if no data after filtering
                    elif (
                        "instrument_id" in data.columns
                        and data["instrument_id"].nunique() == 1
                    ):
                        print(
                            f"[{datetime.now()}] Visualizing data for instrument: {data['instrument_id'].iloc[0]}"
                        )

                    if data.empty:
                        print(
                            f"[{datetime.now()}] Error: After filtering, no MBO data remains from the DBN file '{dbn_filepath}'. Exiting."
                        )
                        return  # Exit if no data after filtering
                    filter_end_time = datetime.now()
                    print(f"[{datetime.now()}] Data filtering took: {filter_end_time - filter_start_time}")

            except Exception as e:
                print(
                    f"[{datetime.now()}] Error reading DBN file '{dbn_filepath}' with databento: {e}. Exiting."
                )
                return  # Exit on error
        else:
            print(
                f"[{datetime.now()}] Error: Not a .dbn file or databento library not found/imported for '{dbn_filepath}'. Exiting."
            )
            return  # Exit if not a DBN file or databento not available

    except Exception as e:
        print(
            f"[{datetime.now()}] An unexpected error occurred during data loading for '{dbn_filepath}': {e}. Exiting."
        )
        return  # Exit on unexpected error

    if data.empty:
        print(
            f"[{datetime.now()}] Error: No data to visualize for '{dbn_filepath}' after all processing. Exiting."
        )
        return

    print(f"[{datetime.now()}] Data loaded successfully. Starting order book processing.")

    # --- 2. Process order book data ---
    # This logic simulates building an order book from MBO messages.
    # For simplicity, we'll aggregate by price. A real MBO book tracks individual orders.
    # This visualization will be more like an MBP (Market By Price) view derived from MBO.

    print(f"[{datetime.now()}] Starting order book processing loop...")
    processing_loop_start_time = datetime.now()
    # Initialize order books and tracking structures
    bid_book = {}  # price -> total_size
    ask_book = {}  # price -> total_size
    active_orders = {}  # order_id -> {'price': price, 'size': size, 'side': side}

    # Map string 'action' and 'side' values to integers for internal processing
    action_map = {
        "A": 0,
        "C": 1,
        "M": 2,
        "T": 3,
        "F": 4,
        "": -1,
    }  # Add, Cancel, Modify, Trade, Fill, Unknown
    side_map = {"B": 0, "A": 1, "N": -1, "": -1}  # Bid, Ask, None, Unknown

    # Lists to store time-series data for mid-price and spread
    timestamps = []
    mid_prices = []
    best_bids = []
    best_asks = []
    spreads = []
    total_bid_quantities = []
    total_ask_quantities = []

    # Initialize total quantities
    current_total_bid_qty = 0
    current_total_ask_qty = 0

    # Initialize for heatmap (if needed, otherwise remove)
    heatmap_timestamps = []
    heatmap_quantities = []

    # Initialize for Cumulative Volume Delta (CVD)
    cvd_timestamps = []
    cvd_values = [0]  # Start CVD at 0

    # Convert price to float if it's an integer (e.g., raw price * 1e-9)
    if "price" in data.columns and data["price"].dtype == "int64":
        # Assuming price is stored as an integer with a fixed scale, e.g., 1e-9
        # This scale factor might need to be determined from DBN metadata or schema.
        # For now, let's assume a common scale or just use the raw price if it looks reasonable.
        # Let's check the magnitude of prices to decide.
        if (
            data["price"].max() > 1_000_000_000
        ):  # Arbitrary large number, suggests scaling
            print(f"[{datetime.now()}] Scaling prices by 1e-9 (assuming raw price format).")
            data["price"] = data["price"] * 1e-9
        else:
            print(f"[{datetime.now()}] Prices appear to be already scaled or in a reasonable range.")
            data["price"] = data["price"].astype(float)

    total_data_rows = len(data)
    # Downsample heatmap_quantities to a fixed number of snapshots to improve performance
    MAX_HEATMAP_SNAPSHOTS = 1000
    heatmap_capture_interval = 1 if total_data_rows <= MAX_HEATMAP_SNAPSHOTS else total_data_rows // MAX_HEATMAP_SNAPSHOTS

    # Downsample time-series data capture to improve performance
    MAX_TIME_SERIES_POINTS = 1000
    time_series_capture_interval = 1 if total_data_rows <= MAX_TIME_SERIES_POINTS else total_data_rows // MAX_TIME_SERIES_POINTS

    # Process each row to build the order book and capture time-series data
    for i, (index, row) in enumerate(data.iterrows()):  # Use i for numerical counter
        # Use 'ts_event' for timestamp if available, otherwise use row index or generate a sequence
        timestamp = row["ts_event"] if "ts_event" in data.columns else index

        order_id = row.get("order_id")  # Assuming 'order_id' exists
        price = row["price"]
        size = row["size"]

        # Convert string side to integer
        side_str = row["side"]
        side = side_map.get(side_str, -1)  # Default to -1 for unknown/None side

        # Convert string action to integer
        action_str = row.get("action", "")
        action = action_map.get(
            action_str, -1
        )  # Default to -1 for unknown/empty action

        # Update active_orders and bid_book/ask_book based on action
        if (
            order_id is not None
        ):  # Only process if order_id is present for accurate book building
            if action == 0:  # Add
                active_orders[order_id] = {
                    "price": price,
                    "size": size,
                    "side": side,
                }
                if side == 0:  # Bid
                    bid_book[price] = bid_book.get(price, 0) + size
                    current_total_bid_qty += size
                elif side == 1:  # Ask
                    ask_book[price] = ask_book.get(price, 0) + size
                    current_total_ask_qty += size
            elif action == 1:  # Cancel/Delete
                if order_id in active_orders:
                    old_order = active_orders.pop(order_id)
                    old_price = old_order["price"]
                    old_size = old_order["size"]
                    old_side = old_order["side"]

                    if old_side == 0:  # Bid
                        bid_book[old_price] -= old_size
                        if bid_book[old_price] <= 0:
                            del bid_book[old_price]
                        current_total_bid_qty -= old_size
                    elif old_side == 1:  # Ask
                        ask_book[old_price] -= old_size
                        if ask_book[old_price] <= 0:
                            del ask_book[old_price]
                        current_total_ask_qty -= old_size
            elif action == 2:  # Modify
                if order_id in active_orders:
                    old_order = active_orders[order_id]
                    old_price = old_order["price"]
                    old_size = old_order["size"]
                    old_side = old_order["side"]

                    # Remove old quantity
                    if old_side == 0:  # Bid
                        bid_book[old_price] -= old_size
                        if bid_book[old_price] <= 0:
                            del bid_book[old_price]
                        current_total_bid_qty -= old_size
                    elif old_side == 1:  # Ask
                        ask_book[old_price] -= old_size
                        if ask_book[old_price] <= 0:
                            del ask_book[old_price]
                        current_total_ask_qty -= old_size

                    # Add new quantity
                    active_orders[order_id] = {
                        "price": price,
                        "size": size,
                        "side": side,
                    }
                    if side == 0:  # Bid
                        bid_book[price] = bid_book.get(price, 0) + size
                        current_total_bid_qty += size
                    elif side == 1:  # Ask
                        ask_book[price] = ask_book.get(price, 0) + size
                        current_total_ask_qty += size
            elif action == 3 or action == 4:  # Trade or Fill (for CVD calculation)
                # Determine if it's a buy or sell trade based on the aggressing side
                # Assuming 'side' in trade/fill messages indicates the aggressor
                if side == 0:  # Bid (aggressing on ask, so it's a buy)
                    cvd_values.append(cvd_values[-1] + size)
                    cvd_timestamps.append(timestamp)
                elif side == 1:  # Ask (aggressing on bid, so it's a sell)
                    cvd_values.append(cvd_values[-1] - size)
                    cvd_timestamps.append(timestamp)
        else:  # Fallback for data without order_id, aggregate directly
            if side == 0:  # Bid
                bid_book[price] = bid_book.get(price, 0) + size
                current_total_bid_qty += size
            elif side == 1:  # Ask
                ask_book[price] = ask_book.get(price, 0) + size
                current_total_ask_qty += size

        # Capture order book state for time-series visualization
        # total_bid_quantities.append(current_total_bid_qty) # Moved inside conditional block
        # total_ask_quantities.append(current_total_ask_qty) # Moved inside conditional block

        if (i % time_series_capture_interval == 0 or i == total_data_rows - 1) and bid_book and ask_book:
            best_bid = max(bid_book.keys())
            best_ask = min(ask_book.keys())

            if best_ask > best_bid:  # Ensure valid spread
                mid_price = (best_bid + best_ask) / 2
                spread = best_ask - best_bid

                timestamps.append(timestamp)
                mid_prices.append(mid_price)
                best_bids.append(best_bid)
                best_asks.append(best_ask)
                spreads.append(spread)
                total_bid_quantities.append(current_total_bid_qty)  # Appended here
                total_ask_quantities.append(current_total_ask_qty)  # Appended here

        # For heatmap, capture snapshots at a lower frequency if needed, or every event
        # For now, let's capture every event for full detail, but this can be optimized
        # if heatmap generation itself becomes a bottleneck.
        if (i % heatmap_capture_interval == 0 or i == total_data_rows - 1) and (bid_book or ask_book):
            current_snapshot = {}
            current_snapshot.update({p: q for p, q in bid_book.items()})
            current_snapshot.update({p: q for p, q in ask_book.items()})
            heatmap_timestamps.append(timestamp)
            heatmap_quantities.append(current_snapshot)


def main():
    parser = argparse.ArgumentParser(
        description="Visualize Databento MBO file as an order book."
    )
    parser.add_argument("dbn_file", help="Path to the Databento DBN file.")
    parser.add_argument(
        "--output_prefix",
        default="order_book_visualization",
        help="Prefix for the output SVG and PNG filenames (e.g., 'my_book' will create my_book.svg and my_book.png).",
    )

    args = parser.parse_args()

    visualize_order_book(args.dbn_file, args.output_prefix)


if __name__ == "__main__":
    main()