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
 * - Geographic coordinate transformations (WGS84 <-> ECEF <-> ENU)
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
/**
 * @section references References
 * 
 * ### Primary References
 * 
 * 1. **World Geodetic System 1984 (WGS84)**  
 *    National Geospatial-Intelligence Agency. (2020).  
 *    *Department of Defense World Geodetic System 1984*.  
 *    https://earth-info.nga.mil/index.php?dir=wgs84&action=wgs84
 * 
 * 2. **G7 Ballistic Standard**  
 *    Sierra Bullets. (2019).  
 *    *Understanding Ballistic Coefficients: G1 vs. G7*.  
 *    https://www.snipershide.com/shooting/threads/understanding-g1-and-g7-ballistic-coefficients.6984110/
 * 
 * ### Technical Standards
 * 
 * - ISO 80000-3:2006, *Quantities and units — Part 3: Space and time*  
 * - ICAO Doc 7488/3, *Manual of the ICAO Standard Atmosphere*  
 * - MIL-STD-2525D, *Common Warfighting Symbology*
 * 
 * ### Online Resources
 * 
 * - Earth-centered, Earth-fixed (ECEF) coordinate system:  
 *   https://en.wikipedia.org/wiki/Earth-centered,_Earth-fixed_coordinate_system
 * - Haversine formula for great-circle distances:  
 *   https://en.wikipedia.org/wiki/Haversine_formula
 * - Runge-Kutta methods for numerical integration:  
 *   https://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods
 * - Ballistic coefficient definitions:  
 *   https://en.wikipedia.org/wiki/Ballistic_coefficient
 * 
 * ### Implementation Notes
 * 
 * This implementation uses:
 * - WGS84 ellipsoid parameters: a = 6378137.0 m, e² = 6.69437999014e-3
 * - G7 drag coefficient approximation as piecewise function of Mach number
 * - 4th-order Runge-Kutta with fixed 20 ms time steps
 * - Constant gravity assumption (9.80665 m/s²)
 * - Sea-level atmospheric density (1.225 kg/m³)
 *
 * @author Horváth Ádám József
 * @date 2025.12.01
 * @version 5.2
 *
 *
 * @todo Add wind effect modeling
 * @todo Implement Coriolis force for long-range trajectories
 * @todo Add binary search for elevation optimization
 * @todo Implement solution caching for performance
 */