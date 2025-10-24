/**
 * Starry Night Urban Skyline - Version 2.0
 * Photorealistic Computational Cityscape with Advanced Lighting Systems
 * Base Implementation: Complex Urban Framework (Chunk 1 Complete)
 * Inspired by Grand Architectural Renaissance and Computational Urbanism
 *
 * Controls:
 * - ESC or mouse/keyboard input to exit
 * - -s F: speed multiplier (default 1.0)
 * - -d N: star density (0=sparse, 1=dense, default 0.5)
 * - -m F: meteor frequency multiplier (default 1.0, higher = more meteors)
 *
 * URBAN FEATURES ACTIVATED:
 * - 11 Building Archetypes with Architectural Authenticity
 * - Advanced Illumination Infrastructure Foundation
 * - Sophisticated Urban Data Architecture
 * - Prepared for Lighting System Integration (Chunks 2-10)
 *
 * Requires: SDL2, mesa/opengl (wayland)
 * Build: gcc -o starrynight2 starrynight2.c -lSDL2 -lGL -lm
 * Run: SDL_VIDEODRIVER=wayland ./starrynight2
 */

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define PI 3.14159265359f
#define STAR_COUNT 500  // Space for drifting sky stars only
#define GAP_STAR_COUNT 10000  // Stars specifically in gaps between buildings
#define METEOR_COUNT 10
#define METEOR_PARTICLES 20
#define CITY_BUILDINGS 13     // Number of solid buildings with windows

// ADVANCED URBAN SYSTEM DEFINES (Chunk 1 Implementation)
#define MAX_URBAN_BUILDINGS 13
#define LIGHTING_SYSTEM_LIMIT 300
#define ROOF_FEATURE_ARRAYS 15

typedef struct {
    float x, y;           // Position in pixels
    float vx, vy;         // Velocity for drift effect
    float brightness;     // Current brightness 0-1 (for twinkling)
    float base_brightness; // Base brightness 0-1
    float twinkle_phase;  // Sin phase offset for unique twinkling
    float twinkle_speed;  // How fast it twinkles
    float size;           // Star size in pixels
    bool is_bright;       // Extra-bright star status for glow effect
    int building_gap;     // Which building gap this star belongs to (-1 for sky)
} Star;

/**
 * MASTER URBAN BUILDING STRUCTURE - COMPLEX ARCHITECTURAL FRAMEWORK
 * Foundation for 11 Building Archetypes with Advanced Urban Features
 */
typedef struct {
    // GENERAL GEOMETRY - Core Structural Elements
    float x, y;                          // Ground floor base position
    float width, height;                 // Primary building envelope
    float right_edge;                    // Calculated boundary coordinates

    // ARCHITECTURAL HIERARCHY - Urban Scale Classification
    int building_type;                   // 0-10 Archetype enumeration
    int floor_quantity;                  // Story count based on archetype
    int window_count_horizontal;         // Windows per level specification

    // ILLUMINATION DYNAMICS - Building-Specific Lighting
    float illumination_percentage;       // 0.0-1.0 occupancy coefficient
    float brightness_coefficient;        // Buildings scale visibility factor
    int illumination_pattern_type;       // Residential/commercial differentiation

    // ROOFTOP INFRASTRUCTURE - Advanced Architectural Accessories
    unsigned int roof_feature_mask;      // Bitmask: antennas, water towers, helipads
    int antenna_element_array;          // Broadcast transmission components
    int water_storage_capacity;         // Municipal utility reservoir status
    int aircraft_warning_beacon_present; // Aviation safety compliance beacons

    // OPERATIONAL STATS - Dynamic Performance Indicators
    float pulse_synchronization_timer;  // Lighting animation coordination
    int beacon_activation_cycle;        // Current flicker sequence phase
    int specialty_feature_indicator;    // Building-classified functionality

    // ADVANCED METRICS - Urban Planning Indicators
    float roof_level_elevation;         // Total height including features
    float architectural_significance;   // Cultural/historical weighting factor

} UrbanBuilding;

// URBAN LIGHTING MASTER CONTROLLER - Dynamic Illumination System
typedef struct {
    float positional_coordinates_x;   // Pixel coordinate precision x
    float positional_coordinates_y;   // Pixel coordinate precision y
    float illumination_intensity;      // Brightness modulation (0-1)
    float temporal_animation_cycle;   // Effects timing synchronization
    int structural_attachment_index;  // Parent building relationship
    int illumination_classification;  // Beacon/sign/etc. enumeration
    int operational_status_flag;      // Active/inactive state management

    // ADVANCED LIGHTING PROPERTIES (Chunk 1 Prepared for Chunks 2-10)
    float spectral_composition_r;     // RGB composition stack
    float spectral_composition_g;
    float spectral_composition_b;
    float operational_duration_ms;   // Duty cycle specifications
    float spatial_influence_radius;  // Lighting effect propagation
    int regulatory_compliance_code;   // Safety/zoning requirement flags

} DynamicLightingElement;

// ROOF ACCESSORY CATALOG DEFINITIONS - Architectural Feature Taxonomy
typedef struct {
    const char* architectural_description;     // ANSI text specification
    int vertical_dimension_requirements;       // Z-axis space allocation
    int illumination_theme_color_primary;      // Dominant spectral allocation
    int illumination_theme_color_secondary;    // Auxiliary lighting scheme
    int collision_detection_volume;           // Physical interference parameters
    int operational_power_consumption;        // Energy demand calculations
    int maintenance_service_interval;         // Scheduled upkeep specifications

} RoofArchitecturalAccessory;

// MASTER URBAN ARRAYS - Foundation for Sophisticated Cityscape
UrbanBuilding urban_complex[MAX_URBAN_BUILDINGS];              // Advanced building repository
DynamicLightingElement illumination_array[LIGHTING_SYSTEM_LIMIT]; // Urban lighting matrix
RoofArchitecturalAccessory architectural_catalog[ROOF_FEATURE_ARRAYS]; // Feature ontologies

// FUNCTION PROTOTYPES - Extended for Urban System Complexity
void init_stars(Star *stars, int count, int screen_width, int screen_height);
void update_stars(Star *stars, int count, float dt, int screen_width, int screen_height);
void render_stars(Star *stars, int count, int screen_width, int screen_height);
void render_gradient_background(int screen_width, int screen_height);
void init_meteor(Meteor *meteor, int screen_width, int screen_height);
void render_meteor(Meteor *meteor, int screen_width, int screen_height);
void update_meteor(Meteor *meteor, float dt, int screen_width, int screen_height);
void init_opengl(int width, int height);

/**
 * URBAN BUILDING INITIALIZATION FUNCTION (Chunk 1 Core Implementation)
 * Generates sophisticated architectural configurations with building archetypes
 */
void initialize_urban_complex_generation(int screen_width, int screen_height) {
    for (int build_index = 0; build_index < MAX_URBAN_BUILDINGS; build_index++) {
        UrbanBuilding* urban_structure = &urban_complex[build_index];

        // SPATIAL POSITIONING - Advanced Urban Layout Algorithm
        float urban_parcel_spacing = (float)screen_width / MAX_URBAN_BUILDINGS;
        float architectural_offset_variance = (float)(rand() % (int)(urban_parcel_spacing * 0.6f) - urban_parcel_spacing * 0.3f);

        // Coordinate system establishment with architectural precision
        urban_structure->x = (float)build_index * urban_parcel_spacing + architectural_offset_variance + 15;
        urban_structure->y = 50.0f; // Ground floor datum consistency

        // ARCHITECTURAL PROFILE DETERMINATION - 11 Building Archetype System
        int architectural_classification = rand() % 11; // Sophisticated typology selection
        urban_structure->building_type = architectural_classification;

        // BUILDING SPECIFICATION ENGINE - Feature-Driven Architectural Synthesis
        switch (architectural_classification) {
            case 0: // RESIDENTIAL APARTMENT COMPLEX (2-10 Stories, Balconies/Balconies Design)
                urban_structure->floor_quantity = 2 + (rand() % 9);
                urban_structure->height = urban_structure->floor_quantity * 20.0f;
                urban_structure->width = 16.0f + (float)(rand() % 16);
                urban_structure->illumination_percentage = 0.7f;
                urban_structure->illumination_pattern_type = 0; // Residential nighttime energy profile
                break;

            case 1: // OFFICE FINANCIAL CENTER (15-35 Stories, Glass Curtain Architecture)
                urban_structure->floor_quantity = 15 + (rand() % 21);
                urban_structure->height = urban_structure->floor_quantity * 18.0f;
                urban_structure->width = 20.0f + (float)(rand() % 25);
                urban_structure->illumination_percentage = 0.9f;
                urban_structure->illumination_pattern_type = 1; // Business hour diurnal cycle
                // Prepare for aircraft beacon installation (Chunk 2)
                urban_structure->aircraft_warning_beacon_present = (urban_structure->floor_quantity >= 30);
                break;

            case 2: // MEGATOWER CONSTRUCTION (40-80 Stories, Supertall Architectural Monument)
                urban_structure->floor_quantity = 40 + (rand() % 41);
                urban_structure->height = urban_structure->floor_quantity * 16.5f;
                urban_structure->width = 24.0f + (float)(rand() % 24);
                urban_structure->illumination_percentage = 1.0f;
                urban_structure->illumination_pattern_type = 2; // 24/7 operational criticality
                urban_structure->aircraft_warning_beacon_present = 1; // Mandatory aviation safety
                urban_structure->antenna_element_array = 1 + (rand() % 3); // Communication infrastructure
                break;

            case 3: // HOSPITAL MEDICAL FACILITY (8-20 Stories, Emergency Illumination)
                urban_structure->floor_quantity = 8 + (rand() % 13);
                urban_structure->height = urban_structure->floor_quantity * 22.0f;
                urban_structure->width = 18.0f + (float)(rand() % 20);
                urban_structure->illumination_percentage = 1.0f;
                urban_structure->illumination_pattern_type = 3; // 24-hour medical operation
                urban_structure->aircraft_warning_beacon_present = (urban_structure->floor_quantity >= 15);
                break;

            case 4: // EDUCATIONAL ACADEMIC INSTITUTE (6-15 Stories, Classroom Configuration)
                urban_structure->floor_quantity = 6 + (rand() % 10);
                urban_structure->height = urban_structure->floor_quantity * 19.0f;
                urban_structure->width = 22.0f + (float)(rand() % 18);
                urban_structure->illumination_percentage = 0.6f;
                urban_structure->illumination_pattern_type = 4; // Academic scheduling cycle
                urban_structure->architectural_significance = 1.2f; // Cultural weighting factor
                break;

            case 5: // COMMERCIAL BUSINESS DISTRICT (10-25 Stories, Neon Advertising)
                urban_structure->floor_quantity = 10 + (rand() % 16);
                urban_structure->height = urban_structure->floor_quantity * 17.5f;
                urban_structure->width = 19.0f + (float)(rand() % 21);
                urban_structure->illumination_percentage = 0.85f;
                urban_structure->illumination_pattern_type = 5; // Late-night commercial economy
                break;

            case 6: // INDUSTRIAL MANUFACTURING COMPLEX (3-8 Stories, Ventilation Systems)
                urban_structure->floor_quantity = 3 + (rand() % 6);
                urban_structure->height = urban_structure->floor_quantity * 25.0f;
                urban_structure->width = 14.0f + (float)(rand() % 18);
                urban_structure->illumination_percentage = 0.8f;
                urban_structure->illumination_pattern_type = 6; // First/third shift operational cycles
                urban_structure->roof_feature_mask |= (1 << ROOF_VENTILATIONS); // Industrial specific
                break;

            case 7: // CULTURAL INSTITUTION VENUE (12-25 Stories, Performance Hall Architecture)
                urban_structure->floor_quantity = 12 + (rand() % 14);
                urban_structure->height = urban_structure->floor_quantity * 20.0f;
                urban_structure->width = 21.0f + (float)(rand() % 19);
                urban_structure->illumination_percentage = 0.4f;
                urban_structure->illumination_pattern_type = 7; // Event-based illumination patterns
                urban_structure->architectural_significance = 1.5f; // Artistic importance multiplier
                break;

            case 8: // RESEARCH LABORATORY COMPLEX (8-18 Stories, Specialized Ventilation)
                urban_structure->floor_quantity = 8 + (rand() % 11);
                urban_structure->height = urban_structure->floor_quantity * 21.0f;
                urban_structure->width = 20.0f + (float)(rand() % 20);
                urban_structure->illumination_percentage = 1.0f;
                urban_structure->illumination_pattern_type = 8; // Continuous operational criticality
                break;

            case 9: // RETAIL SHOPPING COMPLEX (2-6 Stories, Aluminum Framing)
                urban_structure->floor_quantity = 2 + (rand() % 5);
                urban_structure->height = urban_structure->floor_quantity * 28.0f;
                urban_structure->width = 16.0f + (float)(rand() % 24);
                urban_structure->illumination_percentage = 0.75f;
                urban_structure->illumination_pattern_type = 9; // Retail business hour cycle
                break;

            case 10: // CONVENTION EVENT FACILITY (4-12 Stories, Exhibition Architecture)
                urban_structure->floor_quantity = 4 + (rand() % 9);
                urban_structure->height = urban_structure->floor_quantity * 23.0f;
                urban_structure->width = 25.0f + (float)(rand() % 20);
                urban_structure->illumination_percentage = 0.3f;
                urban_structure->illumination_pattern_type = 10; // Convention schedule coordination
                urban_structure->architectural_significance = 1.3f; // Convention center importance
                break;
        }

        // ADVANCED URBAN FEATURE ALLOCATION - Roof Infrastructure Synthesis
        int feature_probability_roll = rand() % 100;

        // Probability-driven roof feature assignment
        if (feature_probability_roll < 25) {
            // Commercial/residential water tower (25% probability)
            urban_structure->water_storage_capacity = 500000 + (rand() % 1500000); // 500K-2M gallon capacity
            urban_structure->roof_feature_mask |= (1 << ROOF_RESERVOIR_TOWER);
        } else if (feature_probability_roll < 40) {
            // Broadcasting antenna array (15% probability)
            urban_structure->antenna_element_array = 1 + (rand() % 5);
            urban_structure->roof_feature_mask |= (1 << ROOF_TRANSMISSION_TOWER);
        } else if (feature_probability_roll < 50) {
            // Helicopter landing platform (10% probability, hospitals/offices)
            urban_structure->roof_feature_mask |= (1 << ROOF_AERIAL_PLATFORM);
        }

        // Specialist building features for high-rise structures
        if (urban_structure->floor_quantity >= 40) {
            urban_structure->roof_feature_mask |= (1 << ROOF_MAINTENANCE_CRANE);
        }

        // Boundary calculation and spatial validation
        urban_structure->right_edge = urban_structure->x + urban_structure->width;
        urban_structure->window_count_horizontal = 2 + (rand() % 6); // Windows per floor variation

        // Roof level elevation calculation for future three-dimensional features
        urban_structure->roof_level_elevation = urban_structure->y + urban_structure->height;

        // Advanced lighting preparation (coordination system for Chunks 2-10)
        urban_structure->pulse_synchronization_timer = (float)rand() / RAND_MAX * 2.0f * PI; // Randomized phase
    }
}

/**
 * URBAN LIGHTING INFRASTRUCTURE INITIALIZATION (Chunk 1 Lighting Framework)
 * Establishes dynamic illumination management system for future chunks
 */
void establish_urban_lighting_infrastructure() {
    // RESET ILLUMINATION ARRAY - Clean slate for sophisticated lighting (Chunks 2-10)
    for (int illumination_index = 0; illumination_index < LIGHTING_SYSTEM_LIMIT; illumination_index++) {
        DynamicLightingElement* light_unit = &illumination_array[illumination_index];

        // Initialize with zero values for system integrity
        light_unit->positional_coordinates_x = light_unit->positional_coordinates_y = 0.0f;
        light_unit->illumination_intensity = 0.0f;
        light_unit->temporal_animation_cycle = 0.0f;
        light_unit->structural_attachment_index = -1;
        light_unit->illumination_classification = 0;
        light_unit->operational_status_flag = 0;
    }

    // ARCHITECTURAL CATALOG POPULATION - Define available roof feature components
    architectural_catalog[ROOF
