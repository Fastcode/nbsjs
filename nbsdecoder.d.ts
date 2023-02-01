/// <reference types="node" />

/**
 * Represents `uint64_t` nanosecond timestamps in JS as an object with separate `seconds` and `nanoseconds` components
 */
export interface NbsTimestamp {
  /** The seconds component of this timestamp */
  seconds: number;

  /** The nanoseconds component of this timestamp */
  nanos: number;
}

/**
 * An NBS packet
 */
export interface NbsPacket {
  /** The NBS packet timestamp */
  timestamp: NbsTimestamp;

  /** The XX64 hash of the packet type */
  type: Buffer;

  /** The packet subtype */
  subtype: number;

  /** The packet data, undefined for empty packets */
  payload?: Buffer;
}

/**
 * A (type, subtype) pair that uniquely identifies a specific type of message
 */
export interface NbsTypeSubtype {
  type: Buffer | string;
  subtype: number;
}

/**
 * A (type, subtype) pair that uniquely identifies a specific type of message,
 * where the type is always a Buffer
 */
export interface NbsTypeSubtypeBuffer extends NbsTypeSubtype {
  type: Buffer;
}

/**
 * A decoder that can be used to read packets from NBS files
 */
export declare class NbsDecoder {
  /**
   * Create a new NbsDecoder instance
   *
   * @param paths A list of absolute paths of nbs files to decode
   * @throws For an empty list of paths, and for paths that don't exist
   */
  public constructor(paths: string[]);

  /** Get a list of all the types present in the loaded nbs files */
  public getAvailableTypes(): NbsTypeSubtypeBuffer[];

  /** Get the timestamp range (start, end) across all packets in the loaded nbs files */
  public getTimestampRange(): [NbsTimestamp, NbsTimestamp];

  /**
   * Get the timestamp range (start, end) for all packets of the given type in the loaded nbs files
   *
   * @param type A type subtype object to get the timestamp range for
   */
  public getTimestampRange(type: NbsTypeSubtype): [NbsTimestamp, NbsTimestamp];

  /**
   * Get the packets at or before the given timestamp for all types in the loaded nbs files
   *
   * Returns a list with one packet for each available type in the loaded nbs files.
   * Empty packets (where `.payload` is undefined) will be returned for types that don't exist
   * for the given timestamp in the loaded nbs files. Empty packets will have their `timestamp`,
   * `type`, and `subtype` set to the corresponding arguments passed to this function.
   *
   * @param timestamp The timestamp to get packets at
   */
  public getPackets(timestamp: number | BigInt | NbsTimestamp): NbsPacket[];

  /**
   * Get the packets at or before the given timestamp for the given types in the loaded nbs files
   *
   * Returns a list of length N for the given list of N types, one packet for each requested type.
   * Empty packets (where `.payload` is undefined) will be returned for types that don't exist
   * for the given timestamp in the loaded nbs files. Empty packets will have their `timestamp`,
   * `type`, and `subtype` set to the corresponding arguments passed to this function.
   *
   * @param timestamp The timestamp to get packets at
   * @param types A list of type subtype objects to get packets for
   */
  public getPackets(
    timestamp: number | BigInt | NbsTimestamp,
    types: NbsTypeSubtype[]
  ): NbsPacket[];

  /**
   *  Get the packets at (n) steps before or after the given timestamp for the given camera type.
   * 
   *  Returns a single timestamp of the specified camera type which is then used in the seek method.
   * 
   * @param timestamp  The timestamp to get packets at.
   * @param type  A type subtype of camera object to get the timestamp.
   * @param steps  Number of steps forwards or backwards to step away from current index.
   * @param flag  To determine which direction, forwards or backs, to step towards. */  
  public stepFrame(
    timestamp: number | BigInt | NbsTimestamp,
    type: NbsTypeSubtype,
    steps: number, 
    stepForwardFlag: boolean
    ): NbsTimestamp;
}
