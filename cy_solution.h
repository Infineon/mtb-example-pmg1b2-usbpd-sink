/***************************************************************************//**
* \file cy_solution.h
* \version 1.0
*
* \brief Header file for solution layer of the PMG1 Code Example.
*
********************************************************************************
* \copyright
* (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#ifndef CY_SOLUTION_H
#define CY_SOLUTION_H

#include "cy_pdstack_dpm.h"
#include <stdbool.h>

#ifndef DEBUG_PRINT_ENABLE
#define DEBUG_PRINT_ENABLE (0)
#endif /* DEBUG_PRINT_ENABLE */

#ifndef DEBUG_PRINT_LOG_ENABLE
#define DEBUG_PRINT_LOG_ENABLE (1)
#endif /* DEBUG_PRINT_LOG_ENABLE */

#if DEBUG_PRINT_ENABLE && (DEBUG_PRINT_LOG_ENABLE == 0)
#define DEBUG_PRINT(string)                         \
{                                                   \
    Cy_SCB_UART_PutString(CYBSP_UART_HW, string);   \
}

#define DEBUG_PRINT_VAR(string, value)              \
{                                                   \
    char temp[80];                                  \
    sprintf(temp,string,value);                     \
    Cy_SCB_UART_PutString(CYBSP_UART_HW, temp);     \
}
#else /* DEBUG_PRINT_ENABLE && (DEBUG_PRINT_LOG_ENABLE == 0) */
#define DEBUG_PRINT(string)
#define DEBUG_PRINT_VAR(string, value)
#endif /* DEBUG_PRINT_ENABLE && (DEBUG_PRINT_LOG_ENABLE == 0) */

#ifndef RVL2
#define RVL2                                        (9100u) /* in Ohm */
#endif /* RVL2 */

#ifndef RIN
#define RIN                                         (9u) /* in mOhm */
#endif /* RIN */

#ifndef RVH2
#define RVH2                                        (45900u) /* in Ohm */
#endif /* RVH2 */

#ifndef RFB
#define RFB                                         (15u) /* in mOhm */
#endif /* RFB */

/* thermistor B coefficient */
#ifndef NTC1_B
#define NTC1_B                                      (4509.0f)
#endif /* NTC1_B */

/* temperature for nominal thermistor resistance */
#ifndef NTC1_T25
#define NTC1_T25                                    (273.0f + 25.0f) /* in Kelvin */
#endif /* NTC1_T25 */

/* thermistor resistance at 25C */
#ifndef NTC1_R25
#define NTC1_R25                                    (100000.0f) /* in Ohm */
#endif /* NTC1_R25 */

/* Pull-up resistor in thermistor divider */
#ifndef NTC1_RP
#define NTC1_RP                                     (40000.0f) /* in Ohm */
#endif /* NTC1_RP */

/* Intermediate define helping to convert resistance into temperature */
#define NTC1_R                                      (NTC1_R25 * expf(-NTC1_B / NTC1_T25))

/*
 * BJT average ADC code - Temperature C
 * 41 - 0 C
 * 39 - 25 C
 * 37 - 60 C
 * 36 - 70 C
 * 35 - 85 C
 * 34 - 100 C
 * 33 - 110 C
 * 32 - 125 C
 */

/* 100; Variation: 85C to 115C */
#ifndef BJT_OTP_CODE
#define BJT_OTP_CODE                                (34u)
#endif /* BJT_OTP_CODE */

/* 1 ADC BJT code */
#ifndef BJT_HYST_CODE
#define BJT_HYST_CODE                               (1u)
#endif /* BJT_HYST_CODE */

/*
 * Activity indicator LED timer. The timer is used to indicate that the firmware
 * is functional. The feature is controlled by APP_FW_LED_ENABLE.
 */
#define LED_TIMER_ID                                (CY_PDUTILS_TIMER_USER_START_ID)

/* The LED toggle period (ms) to be used when BB enabled. */
#ifndef LED_PD_SNK_TIMER_PERIOD
#define LED_PD_SNK_TIMER_PERIOD                     (100u)
#endif /* LED_PD_SNK_TIMER_PERIOD */

#define BB_MONITOR_TASK_TIMER_ID                    (CY_PDUTILS_TIMER_USER_START_ID + 1u)
#ifndef BB_MONITOR_TASK_TIMER_PERIOD
#define BB_MONITOR_TASK_TIMER_PERIOD                (100u)
#endif /* BB_MONITOR_TASK_TIMER_PERIOD */


#define SOFTSTART_FET_IN_TIMER_ID                   (CY_PDUTILS_TIMER_USER_START_ID + 2u)
#ifndef SOFTSTART_FET_IN_TIMER_PERIOD
#define SOFTSTART_FET_IN_TIMER_PERIOD               (1200u)
#endif /* SOFTSTART_FET_IN_TIMER_PERIOD */


#define PWMI_TIMER_ID                               (CY_PDUTILS_TIMER_USER_START_ID + 3u)
#ifndef PWMI_TIMER_PERIOD
#define PWMI_TIMER_PERIOD                           (5u)
#endif /* PWMI_TIMER_PERIOD */


#define RECT_TIMER_ID                               (CY_PDUTILS_TIMER_USER_START_ID + 4u)
#ifndef RECT_TIMER_PERIOD
#define RECT_TIMER_PERIOD                           (PWMI_TIMER_PERIOD + 10u)
#endif /* RECT_TIMER_PERIOD */


/* Timer controls how frequently ADC measures In/Out voltages/currents */
#define ADC_PERIOD_TIMER_ID                         (CY_PDUTILS_TIMER_USER_START_ID + 5u)
/* Period (in ms) of ADC measurements for averaging */
#define ADC_PERIOD_TIMER_PERIOD                     (2u)


/* Timer to wait unexpected VOUT presence before enabled BB */
#define VOUT_UNEXP_TIMER_ID                         (CY_PDUTILS_TIMER_USER_START_ID + 6u)
/* Period (in ms) of unexpected VOUT */
#define VOUT_UNEXP_TIMER_PERIOD                     (10000u)


/* Timer to turn off PFET before enable BB discharge */
#define PFET_TURN_OFF_TIMER_ID                      (CY_PDUTILS_TIMER_USER_START_ID + 7u)
/* Period (in ms) of turn off */
#define PFET_TURN_OFF_TIMER_PERIOD                  (5u)


/* Timer to discharge input BB capacitors */
#define BB_DSCHRG_TIMER_ID                          (CY_PDUTILS_TIMER_USER_START_ID + 8u)
/* Period (in ms) of discharge */
#define BB_DSCHRG_TIMER_PERIOD                      (10u)


/* VBUS OCP percentage for BB Input Current  */
#ifndef VBUS_OCP_CURR_PER
#define VBUS_OCP_CURR_PER                           (20u)
#endif /* VBUS_OCP_CURR_PER */


/* NTC1 OTP threshold in C */
#ifndef NTC1_OTP_THRES
#define NTC1_OTP_THRES                             (100)
#endif /* NTC1_OTP_THRES */


/* NTC1 temperature hysteresis in C */
#ifndef NTC1_TEMP_HYST
#define NTC1_TEMP_HYST                             (10)
#endif /* NTC1_TEMP_HYST */


/* VOUT UVP threshold in mV */
#ifndef VOUT_UVP_VOLT
#define VOUT_UVP_VOLT                              (16000u)
#endif /* VOUT_UVP_VOLT */


/* VOUT OVP threshold in mV */
#ifndef VOUT_OVP_VOLT
#define VOUT_OVP_VOLT                              (24000u)
#endif /* VOUT_OVP_VOLT */


/* Maximum allowed VOUT before Buck-Boost initialization in mV. The output is
 * expected to be discharged prior to enabling the Buck-Boost. A measurement
 * above this threshold indicates an unexpected output voltage condition. */
#ifndef VOUT_INIT_MAX_VOLT
#define VOUT_INIT_MAX_VOLT                         (2000u)
#endif /* VOUT_INIT_MAX_VOLT */


/* minimal TYPEC contract power BB requires in mW */
#ifndef TYPEC_MIN_PWR
#define TYPEC_MIN_PWR                              (15000u)
#endif /* TYPEC_MIN_PWR */


/**
 * @brief Commands to control BB and input FET
 */
typedef enum
{
    CTRL_BB_DIS = 0,
    CTRL_BB_EN,
    CTRL_BB_IN_EN,
    CTRL_BB_IN_DIS,
    CTRL_BB_RST,
    CTRL_BB_DSCHRG
} bb_control_cmd_t;

/**
 * @brief Enumeration for temperature states
 */
typedef enum temp_state
{
    STATE_TEMP_NORMAL = 0,
    STATE_TEMP_OTP
} temp_state_t;


/**  No events pending. */
#define PMG_EVT_NONE                             (0x00000000u)

/** State change pending. */
#define PMG_EVT_ENTRY                            (0x00000001u)

/** BB input FET soft-start complete. */
#define PMG_EVT_IN_SS_CMLT                       (0x00000002u)

/** Mask all events. */
#define PMG_EVT_ALL_MASK                         (0xFFFFFFFFu)

/**
 * @typedef pmg_fsm_evt_t
 * @brief List of PMG events.
 */
typedef enum
{
    PMG_FSM_EVT_ENTRY = 0,             /**<  0: PMG Event: State entry. */
    PMG_FSM_EVT_IN_SS_CMLT,            /**<  1: PMG Event: BB Input switch soft-start complete. */
    PMG_FSM_EVT_MAX_EVTS               /**<  2: Number of events. */
} pmg_fsm_evt_t;

/**
 * @typedef pmg_state_t
 * @brief List of PMG BB states in the state machine.
 */
typedef enum
{
    PMG_FSM_OFF = 0,                  /**< PMG state machine inactive. */
    PMG_FSM_INIT,
    PMG_FSM_INIT_FET_IN,
    PMG_FSM_INIT_BB,
    PMG_FSM_BB_COMPLETE,
    PMG_FSM_DELIVERY,
    PMG_FSM_SHUTDOWN,
    PMG_FSM_COOLDOWN,                 /**< PMG state to wait cool-down. */
    PMG_FSM_MAX_STATES                /**< Invalid state ID. */
} pmg_state_t;

/** Structure to hold BB faults. */
typedef struct
{
    uint32_t bb_sw_ovp        : 1;  /**< Bit 0: BB SW OVP fault condition flag */
    uint32_t bb_sw_ocp        : 1;  /**< Bit 1: BB SW OCP fault condition flag */
    uint32_t bb_sw_uvp        : 1;  /**< Bit 2: BB SW UVP fault condition flag */
    uint32_t ntc1_otp         : 1;  /**< Bit 3: NTC1 OTP fault condition flag */
    uint32_t bb_hw_ovp        : 1;  /**< Bit 4: BB HW OVP fault condition flag */
    uint32_t bb_hw_ocp        : 1;  /**< Bit 5: BB HW OCP fault condition flag */
    uint32_t bb_hw_otp        : 1;  /**< Bit 6: BB HW OTP fault condition flag */
    uint32_t bjt_otp          : 1;  /**< Bit 7: BJT OTP fault condition flag */
    uint32_t bb_vout_init     : 1;  /**< Bit 8: Unexpected VOUT before BB init flag */
    uint32_t fault_active     : 1;  /**< Bit 9: Latched fault indication */
    uint32_t reserved         : 22; /**< Bits 31:10 - Reserved for future use */
} pmg_fault_t;

/**
 * @brief Structure holds the Buck-Boost Status
 */
typedef struct cy_stc_status
{
    bool is_vbus_pwr_sufficient;       /**< VBUS Power is sufficient to BB operation */
    pmg_fault_t faults;                /**< Buck-Boost faults */
    uint32_t bb_power_in;              /**< Input power available for Buck-Boost */
    uint16_t bb_v_out_mon;             /**< Current BB output voltage measured by ADC */
    uint16_t bb_i_in_mon;              /**< Current BB input current measured in 10mA units */
    int16_t ntc1_temp;                 /**< Temperature in degree Celsius measured by NTC1 sensor */
    temp_state_t ntc1_state;           /**< NTC1 state machine */
    uint8_t bjt_code;                  /**< BJT (silicon temperature sensor) ADC code */
    temp_state_t bjt_state;            /**< BJT state machine */
    pmg_state_t pmg_fsm_state;         /**< Current state of the PMG state machine */
    uint32_t pmg_evt;                  /**< Bitmap representing event notifications to the state
                                            machine */
    uint8_t ef1;                       /**< Error flag 1 read from BB */
    uint8_t ef2;                       /**< Error flag 2 read from BB */
    uint16_t vddd;                     /**< VDDD voltage in mV */
    bool bb_in_fet_status;             /**< BB input FET status */
} cy_stc_status_t;

/**
 * @brief Structure contains ADC measured values
 */
typedef struct cy_stc_measure
{
    uint32_t index;                    /**< Filter Index */
    uint32_t i_in_mon_sum;             /**< Measured input BB current */
    float ntc1_sum;                    /**< NTC1 sensor temperature */
    uint32_t v_out_mon_sum;            /**< Measured output BB voltage */
    uint32_t bjt_code_sum;             /**< BJT (Silicon temperature sensor) ADC code */
    uint32_t vddd_sum;                 /**< VDDD voltage in mV */
} cy_stc_measure_t;


/**
 * @brief Structure for Buck-Boost context information.
 */
typedef struct cy_stc_bb_context
{
    cy_stc_status_t status;                 /**< Status structure */
    cy_stc_measure_t measure;               /**< Measure structure */
    cy_stc_pdstack_context_t* ptrPdStack;   /**< Pointer to PD stack */
} cy_stc_bb_context_t;

/*******************************************************************************
 * Global Function Declaration
 ******************************************************************************/

/**
 * @brief This function initializes solution status structure.
 * @param ptrPdStackContext PD Stack context.
 */
void Soln_BB_Init(cy_stc_pdstack_context_t* ptrPdStackContext);

/**
 * @brief Buck-Boost Solution PD event handler .
 * @param ptrPdStackContext PD Stack context.
 * @param evt PD event
 * @param data Pointer to PD event data
 */
void Soln_BB_PdEventHandler(cy_stc_pdstack_context_t* ptrPdStackContext,
                               cy_en_pdstack_app_evt_t evt, const void* data);

/**
 * @brief This function processes Solution level tasks.
 * @param ptrPdStackContext PD Stack context.
 */
void Soln_BB_Task(cy_stc_pdstack_context_t* ptrPdStackContext);


#endif /* CY_SOLUTION_H */

/* [] END OF FILE */
