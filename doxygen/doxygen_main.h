/**
 * @mainpage Ballistic Trajectory Calculator
 * 
 * @section overview Overview
 * A comprehensive ballistic calculation system for artillery fire control.
 * The program computes firing solutions, manages military bases, targets,
 * and ammunition inventory, and provides optimal base-ammunition selection.
 * 
 * @section features Features
 * - Ballistic trajectory simulation using WGS84 Earth model
 * - G7 drag coefficient model for projectile aerodynamics
 * - 4th-order Runge-Kutta numerical integration
 * - Geographic coordinate transformations (WGS84 ↔ ECEF ↔ ENU)
 * - Linked list data structures for all entities
 * - Configuration file support with persistent settings
 * - Color-coded terminal output (ANSI escape codes)
 * - Memory leak detection via debugmalloc
 * - Interactive command-line interface
 * - File I/O for data persistence
 * - Regional awareness for base selection (ID-based regions)
 * 
 * @section architecture Architecture
 * The system is organized into the following modules:
 * - @ref Storage: Core data structures and linked list implementations
 * - @ref Loading: File parsing, configuration, and data loading
 * - @ref Calculations: Ballistic computations and coordinate transformations
 * - @ref CommandHandling: Interactive command processing and user interface
 * - @ref Utilities: Helper functions, error handling, and color management
 * - @ref Main: Program lifecycle and main control flow
 * 
 * @section usage Usage
 * ### Basic Operation:
 * 1. Program starts and loads configuration from `config.txt`
 * 2. Data files (`ammo_catalouge.txt`, `bases.txt`, `targets.txt`) are loaded
 * 3. Interactive command prompt appears
 * 4. Use commands like `calc` to compute firing solutions
 * 5. Type `help` for available commands, `exit` to quit
 * 
 * ### Data Format Examples:
 * - Ammo: `I0 NHE W50.0 D120.0 BR10.0 MV800.0 BC0.5;`
 * - Base: `I0 LBase1 CLA47.4729283 CLO19.0531429 CA100.0 AMS:{0:100,1:50};`
 * - Target: `I0 LTarget1 CLA47.5 CLO19.1 CA50.0 A1.5;`
 * 
 * @section technical_details Technical Details
 * - **Earth Model**: WGS84 ellipsoid with ECEF/ENU coordinate systems
 * - **Ballistics**: G7 drag model, constant gravity (9.80665 m/s²)
 * - **Integration**: 4th-order Runge-Kutta with 20ms time steps
 * - **Search**: 0.1° elevation increments from 5° to 80°
 * - **Tolerance**: 1 meter for distance and altitude matching
 * - **Memory**: Sentinel-linked lists with head/tail nodes
 * 
 * @author Horváth Ádám József
 * @date 2025
 * @version 5.2
 * 
 * @todo Add wind effect modeling
 * @todo Implement Coriolis force for long-range trajectories
 * @todo Add binary search for elevation optimization
 * @todo Implement solution caching for performance
 */