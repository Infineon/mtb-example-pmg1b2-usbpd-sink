/***************************************************************************//**
* \file cy_solution.c
* \version 1.0
*
* \brief Source file for solution layer of the PMG1 Code Example.
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

#include "cy_pdutils.h"
#include "cy_solution.h"
#include "cybsp.h"
#include <stdint.h>
#include <stdbool.h>
#if defined(DEBUG_PRINT_ENABLE) && (DEBUG_PRINT_ENABLE)
#include <stdio.h>
#endif /* defined(DEBUG_PRINT_ENABLE) && (DEBUG_PRINT_ENABLE) */
#include "cy_gpio.h"
#include <math.h>
#if defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE)
#include "cy_app_led_ctrl.h"
#endif /* defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE) */


#define IINMON_IN_PORT GPIO_PRT2
#define IINMON_IN_PIN 0U

#define VBUS_MON_PORT GPIO_PRT2
#define VBUS_MON_PIN 1U

#define EF1_IN_PORT GPIO_PRT2
#define EF1_IN_PIN 4U

#define EF2_IN_PORT GPIO_PRT2
#define EF2_IN_PIN 5U

#define PWMI_OUT_PORT GPIO_PRT2
#define PWMI_OUT_PIN 6U

#define VBAT_MON_PORT GPIO_PRT4
#define VBAT_MON_PIN 0U


#if defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE)
static cy_stc_led_ctrl_context_t led_context;
static void pmg_led_mngr(void);
#endif /* defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE) */

/* Kelvin to Celsius offset */
#define PMG_KELVIN_TO_CELSIUS_OFFSET    (273.0f)

/* Debug print buffer size */
#define PMG_DEBUG_PRINT_BUFFER_SIZE     (80u)

#define IIN_SENSE_GAIN                  (20u)
#define MA_PER_CURRENT_STEP             (1000u / CY_PD_CUR_PER_UNIT)
#define AMUX_SETTLE_DELAY_US            (20u)
#define GPIO_RESTORE_DELAY_US           (10u)
#define BB_RESET_ASSERT_DELAY_US        (100u)
#define BB_RESET_RECOVERY_DELAY_US      (150u)
#define EF_DEBOUNCE_DELAY_US            (100u)
#define PERCENT_DIVISOR                 (100u)


cy_stc_bb_context_t bb_context;

/* PMG FSM prototypes. */
static void bb_fsm_off(const uint8_t evt);
static void bb_fsm_init(const uint8_t evt);
static void bb_fsm_init_fet_in(const uint8_t evt);
static void bb_fsm_init_bb(const uint8_t evt);
static void bb_fsm_bb_complete(const uint8_t evt);
static void bb_fsm_delivery(const uint8_t evt);
static void bb_fsm_shutdown(const uint8_t evt);
static void bb_fsm_cooldown(const uint8_t evt);

static void bb_control_cmd(const bb_control_cmd_t src_cmd);
static void bb_disable(void);


/* Creates an entry event into next FSM state. */
static void Soln_BB_Next_Fsm_Entry(pmg_state_t next_state)
{
    bb_context.status.pmg_evt |= PMG_EVT_ENTRY;
    bb_context.status.pmg_fsm_state = next_state;
}


static void bb_fsm_off(const uint8_t evt)
{
    /* Nothing to do in this state. */
    CY_UNUSED_PARAMETER(evt);
}


static void bb_fsm_init(const uint8_t evt)
{
    if (evt == PMG_FSM_EVT_ENTRY)
    {
        Soln_BB_Next_Fsm_Entry(PMG_FSM_INIT_FET_IN);
    }
}


static void bb_fsm_init_fet_in(const uint8_t evt)
{
    if (evt == PMG_FSM_EVT_ENTRY)
    {
        bb_control_cmd(CTRL_BB_IN_EN);
    }
    if (evt == PMG_FSM_EVT_IN_SS_CMLT)
    {
        Soln_BB_Next_Fsm_Entry(PMG_FSM_INIT_BB);
    }
}


static void bb_fsm_init_bb(const uint8_t evt)
{
    cy_stc_status_t* stat = &(bb_context.status);

    if (evt == PMG_FSM_EVT_ENTRY)
    {
        stat->faults.bb_vout_init = 0u;
        Cy_PdUtils_SwTimer_Stop(bb_context.ptrPdStack->ptrTimerContext,
                                VOUT_UNEXP_TIMER_ID);
    }

    /*
     * Before enabling the Buck-Boost the output is expected to be
     * discharged. If VOUT is above the threshold it indicates an
     * unexpected output voltage condition; flag the fault and wait
     * instead of enabling the Buck-Boost.
     */
    if (stat->bb_v_out_mon < VOUT_INIT_MAX_VOLT)
    {
        stat->faults.bb_vout_init = 0u;
        Cy_PdUtils_SwTimer_Stop(bb_context.ptrPdStack->ptrTimerContext,
                                VOUT_UNEXP_TIMER_ID);
        bb_control_cmd(CTRL_BB_RST);
        bb_control_cmd(CTRL_BB_EN);
        Soln_BB_Next_Fsm_Entry(PMG_FSM_BB_COMPLETE);
    }
    else
    {
        if (Cy_PdUtils_SwTimer_IsRunning(bb_context.ptrPdStack->ptrTimerContext,
                                         VOUT_UNEXP_TIMER_ID) == false)
        {
            if (stat->faults.bb_vout_init == 0u)
            {
                stat->faults.bb_vout_init = 1u;
                DEBUG_PRINT("\r\n Unexpected VOUT before BB init");
                (void)Cy_PdUtils_SwTimer_StartWocb(bb_context.ptrPdStack->ptrTimerContext,
                                                   VOUT_UNEXP_TIMER_ID,
                                                   VOUT_UNEXP_TIMER_PERIOD);
            }
            else
            {
                DEBUG_PRINT("\r\n Go to shutdown due to unexpected VOUT");
                Soln_BB_Next_Fsm_Entry(PMG_FSM_SHUTDOWN);
            }
        }
    }
}


static void bb_fsm_bb_complete(const uint8_t evt)
{
    if (evt == PMG_FSM_EVT_ENTRY)
    {
        Soln_BB_Next_Fsm_Entry(PMG_FSM_DELIVERY);
    }
}


static void bb_fsm_delivery(const uint8_t evt)
{
    CY_UNUSED_PARAMETER(evt);
}


static void bb_fsm_shutdown(const uint8_t evt)
{
    if (evt == PMG_FSM_EVT_ENTRY)
    {
        bb_disable();
    }
}


static void bb_fsm_cooldown(const uint8_t evt)
{
    const cy_stc_status_t* stat = &(bb_context.status);

    if (evt == PMG_FSM_EVT_ENTRY)
    {
        bb_control_cmd(CTRL_BB_DIS);
    }
    if ((stat->faults.bjt_otp == 0u) && (stat->faults.ntc1_otp == 0u))
    {
        Soln_BB_Next_Fsm_Entry(PMG_FSM_INIT_BB);
    }
}


void Soln_BB_Init(cy_stc_pdstack_context_t* ptrPdStackContext)
{
    bb_context.ptrPdStack = ptrPdStackContext;

    /*
     * This is a temporary solution. In future has to be generated by Device configurator
     */
    HSIOM->AMUX_SPLIT_CTL[0] = HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SL_Msk |
                               HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SR_Msk;
    HSIOM->AMUX_SPLIT_CTL[1] = HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SL_Msk |
                               HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SR_Msk;

    #if defined(VBUS_EXTERNAL_DISCHARGE) && (VBUS_EXTERNAL_DISCHARGE)
    /* VBUS_Discharge pin configuration */
    Cy_USBPD_Vbus_ExtDischargeInit();
    #endif /* defined(VBUS_EXTERNAL_DISCHARGE) && (VBUS_EXTERNAL_DISCHARGE) */

    /*
     * To measure VBUS_MON the PDL mostly uses ID_0 and AMUXA.
     * bb_v_out_mon, IIN_MON, shell use ID_1 and AMUXB.
     */
    Cy_GPIO_Pin_FastInit(IINMON_IN_PORT, IINMON_IN_PIN, CY_GPIO_DM_ANALOG, 1u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(VBUS_MON_PORT, VBUS_MON_PIN, CY_GPIO_DM_ANALOG, 1u, HSIOM_SEL_AMUXA);
    Cy_GPIO_Pin_FastInit(EF1_IN_PORT, EF1_IN_PIN, CY_GPIO_DM_HIGHZ, 1u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(EF2_IN_PORT, EF2_IN_PIN, CY_GPIO_DM_HIGHZ, 1u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(PWMI_OUT_PORT, PWMI_OUT_PIN, CY_GPIO_DM_STRONG_IN_OFF, 0u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(VBAT_MON_PORT, VBAT_MON_PIN, CY_GPIO_DM_ANALOG, 1u, HSIOM_SEL_GPIO);

    #if defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE)
    led_context.port = UART_RX_PORT;
    led_context.pinNum = UART_RX_PIN;
    led_context.ledOrientation = false;
    led_context.timerId = LED_TIMER_ID;
    led_context.timerContext = ptrPdStackContext->ptrTimerContext;
    Cy_App_LedCtrl_Init(&led_context);
    #endif /* defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE) */
}


static void faults_handler(void)
{
    const cy_stc_status_t* stat = &(bb_context.status);

    if ((stat->faults.bb_sw_ocp != 0u) ||
        (stat->faults.bb_sw_ovp != 0u) ||
        (stat->faults.bb_sw_uvp != 0u) ||
        (stat->faults.bb_hw_ocp != 0u) ||
        (stat->faults.bb_hw_ovp != 0u))
    {
        if (stat->pmg_fsm_state != PMG_FSM_OFF)
        {
            Soln_BB_Next_Fsm_Entry(PMG_FSM_SHUTDOWN);
        }
    }
    else if ((stat->faults.bjt_otp != 0u) ||
             (stat->faults.ntc1_otp != 0u))
    {
        if (stat->pmg_fsm_state == PMG_FSM_DELIVERY)
        {
            /* If the device overheats go to cool-down state. */
            Soln_BB_Next_Fsm_Entry(PMG_FSM_COOLDOWN);
        }
    }
    else
    {
        /* Do nothing, no faults detected. */
    }
}

static uint16_t get_adc_result_mv(const GPIO_PRT_Type* port, const uint32_t pin)
{
    uint32_t volt;

    Cy_GPIO_SetHSIOM(port, pin, HSIOM_SEL_AMUXB);
    Cy_SysLib_DelayUs(AMUX_SETTLE_DELAY_US);

    /* Take ADC sample. */
    volt = Cy_USBPD_Adc_Sample(bb_context.ptrPdStack->ptrUsbPdContext, CY_USBPD_ADC_ID_1, CY_USBPD_ADC_INPUT_AMUX_B);
    volt *= bb_context.ptrPdStack->ptrUsbPdContext->adcVdddMv[CY_USBPD_ADC_ID_1];
    volt /= PD_ADC_NUM_LEVELS;

    Cy_GPIO_SetHSIOM(port, pin, HSIOM_SEL_GPIO);
    Cy_SysLib_DelayUs(GPIO_RESTORE_DELAY_US);

    return (uint16_t)(volt);
}

static void adc_measurements(void)
{
    cy_stc_measure_t* adcResult = &(bb_context.measure);
    cy_stc_pdstack_context_t* ptrPdStackContext = bb_context.ptrPdStack;

    uint8_t level = Cy_USBPD_Adc_Sample(ptrPdStackContext->ptrUsbPdContext, CY_USBPD_ADC_ID_1,
                                        CY_USBPD_ADC_INPUT_BANDGAP);

    /* Check for zero. If level came out as zero, then do not calculate. */
    if (level != 0U)
    {
        uint32_t vddd = PD_ADC_BAND_GAP_VOLT_MV * PD_ADC_NUM_LEVELS;
        vddd /= level;
        ptrPdStackContext->ptrUsbPdContext->adcVdddMv[CY_USBPD_ADC_ID_1] = vddd;
        adcResult->vddd_sum += vddd;

        uint32_t i_in_mon = get_adc_result_mv(IINMON_IN_PORT, IINMON_IN_PIN);
        i_in_mon *= MA_PER_CURRENT_STEP;
        i_in_mon /= (RIN * IIN_SENSE_GAIN);
        adcResult->i_in_mon_sum += i_in_mon;

        uint32_t vout = get_adc_result_mv(VBAT_MON_PORT, VBAT_MON_PIN);
        vout *= (RVL2 + RVH2);
        vout /= RVL2;
        adcResult->v_out_mon_sum += vout;

        uint32_t ntc1 = get_adc_result_mv(NTC1_PORT, NTC1_PIN);
        if (vddd > ntc1)
        {
            uint32_t ntc1_mv = (uint32_t)NTC1_RP;
            ntc1_mv *= ntc1;
            ntc1_mv /= (vddd - ntc1);
            adcResult->ntc1_sum += ntc1_mv;
        }

        adcResult->bjt_code_sum += Cy_USBPD_Adc_Sample(ptrPdStackContext->ptrUsbPdContext,
                                                       CY_USBPD_ADC_ID_1, CY_USBPD_ADC_INPUT_BJT);
    }
}


static void read_bb_error_flag(void)
{
    cy_stc_status_t* stat = &(bb_context.status);

    /* Do not update EFx during BB discharge as faults expected */
    if (Cy_PdUtils_SwTimer_IsRunning(bb_context.ptrPdStack->ptrTimerContext,
                                     (cy_timer_id_t)BB_DSCHRG_TIMER_ID) == false)
    {
        uint8_t ef1 = (uint8_t)Cy_GPIO_Read(EF1_IN_PORT, EF1_IN_PIN);
        uint8_t ef2 = (uint8_t)Cy_GPIO_Read(EF2_IN_PORT, EF2_IN_PIN);

        if ((ef1 == 0u) || (ef2 == 0u))
        {
            Cy_SysLib_DelayUs(EF_DEBOUNCE_DELAY_US);
            ef1 = (uint8_t)Cy_GPIO_Read(EF1_IN_PORT, EF1_IN_PIN);
            ef2 = (uint8_t)Cy_GPIO_Read(EF2_IN_PORT, EF2_IN_PIN);
        }

        stat->ef1 = ef1;
        stat->ef2 = ef2;
    }
    #if 0
    DEBUG_PRINT_VAR("\r\n EF1:: %d ", stat->ef1);
    DEBUG_PRINT_VAR(" EF2:: %d", stat->ef2);
    #endif

    if (stat->ef1 == 0u)
    {
        if (stat->ef2 != 0u)
        {
            stat->faults.bb_hw_ocp = 1u;
        }
        else
        {
            stat->faults.bb_hw_otp = 1u;
        }
    }
    else
    {
        if (stat->ef2 == 0u)
        {
            stat->faults.bb_hw_ovp = 1u;
        }
        else
        {
            stat->faults.bb_hw_ovp = 0u;
            stat->faults.bb_hw_ocp = 0u;
            stat->faults.bb_hw_otp = 0u;
        }
    }
}


static void output_voltage_monitor(void)
{
    cy_stc_status_t* stat = &(bb_context.status);

    if (stat->pmg_fsm_state == PMG_FSM_DELIVERY) 
    {
        DEBUG_PRINT_VAR("\r\n Output voltage: %d mV", stat->bb_v_out_mon);

        if (stat->bb_v_out_mon <= VOUT_UVP_VOLT)
        {
            stat->faults.bb_sw_uvp = 1u;
            DEBUG_PRINT("\r\n Output is in UVP");
        }

        if (stat->bb_v_out_mon >= VOUT_OVP_VOLT)
        {
            stat->faults.bb_sw_ovp = 1u;
            DEBUG_PRINT("\r\n Output is in OVP");
        }
    }
}


static void current_monitor(void)
{
    cy_stc_status_t* stat = &(bb_context.status);
    uint32_t bb_input_cur_ocp_limit = bb_context.ptrPdStack->dpmStat.contract.curPwr;
    bb_input_cur_ocp_limit *= (PERCENT_DIVISOR + VBUS_OCP_CURR_PER);
    bb_input_cur_ocp_limit /= PERCENT_DIVISOR;

    DEBUG_PRINT_VAR("\r\n Input current: %d mA", stat->bb_i_in_mon * CY_PD_CUR_PER_UNIT);

    if (stat->bb_i_in_mon > bb_input_cur_ocp_limit)
    {
        stat->faults.bb_sw_ocp = 1u;
        DEBUG_PRINT("\r\n VBUS OCP detected");
    }
    else
    {
        stat->faults.bb_sw_ocp = 0u;
    }
}


static void measure_onboard_ntc1_sensor_data(void)
{
    cy_stc_status_t* stat = &(bb_context.status);

    DEBUG_PRINT_VAR("\r\n ntc1:: %d C", stat->ntc1_temp);

    switch (stat->ntc1_state)
    {
        case STATE_TEMP_NORMAL:
            if (stat->ntc1_temp >= NTC1_OTP_THRES)
            {
                stat->faults.ntc1_otp = 1u;
                stat->ntc1_state = STATE_TEMP_OTP;
                DEBUG_PRINT("\r\n NTC1 OTP");
            }
            break;

        case STATE_TEMP_OTP:
            if (stat->ntc1_temp < (NTC1_OTP_THRES - NTC1_TEMP_HYST))
            {
                stat->faults.ntc1_otp = 0u;
                stat->ntc1_state = STATE_TEMP_NORMAL;
                DEBUG_PRINT("\r\n NTC1 normal");
            }
            break;

        default:
            break;
    }
}


static void measure_bjt_sensor_data(void)
{
    cy_stc_status_t* stat = &(bb_context.status);

    switch (stat->bjt_state)
    {
        case STATE_TEMP_NORMAL:
            if (stat->bjt_code <= BJT_OTP_CODE)
            {
                stat->faults.bjt_otp = 1u;
                stat->bjt_state = STATE_TEMP_OTP;
                DEBUG_PRINT("\r\n BJT OTP");
            }
            break;

        case STATE_TEMP_OTP:
            if (stat->bjt_code > (BJT_OTP_CODE + BJT_HYST_CODE))
            {
                stat->faults.bjt_otp = 0u;
                stat->bjt_state = STATE_TEMP_NORMAL;
                DEBUG_PRINT("\r\n BJT normal");
            }
            break;

        default:
            break;
    }
}


static void bb_monitor(void)
{
    cy_stc_status_t* stat = &(bb_context.status);

    read_bb_error_flag();
    measure_onboard_ntc1_sensor_data();
    measure_bjt_sensor_data();
    output_voltage_monitor();

    /* Monitor current delivery mode */
    if ((stat->pmg_fsm_state == PMG_FSM_DELIVERY) &&
        (stat->is_vbus_pwr_sufficient == true) &&
        (stat->faults.bb_sw_ocp == 0u))
    {
        current_monitor();
    }
    else
    {
        stat->bb_i_in_mon = 0u;
    }
}


static void output_process(void)
{
    static void(*const bb_fsm_table[PMG_FSM_MAX_STATES]) (const uint8_t evt) =
    {
        bb_fsm_off,                                /*  0: PMG_FSM_OFF */
        bb_fsm_init,                               /*  1: PMG_FSM_INIT */
        bb_fsm_init_fet_in,                        /*  2: PMG_FSM_INIT_FET_IN */
        bb_fsm_init_bb,                            /*  3: PMG_FSM_INIT_BB */
        bb_fsm_bb_complete,                        /*  4: PMG_FSM_BB_COMPLETE */
        bb_fsm_delivery,                           /*  5: PMG_FSM_DELIVERY */
        bb_fsm_shutdown,                           /*  6: PMG_FSM_SHUTDOWN */
        bb_fsm_cooldown,                           /*  7: PMG_FSM_COOLDOWN */
    };

    cy_stc_status_t* stat = &(bb_context.status);
    cy_stc_measure_t* adcResult = &(bb_context.measure);
    uint8_t evt;

    stat->bb_v_out_mon = (uint16_t)(adcResult->v_out_mon_sum / adcResult->index);
    stat->bb_i_in_mon = (uint16_t)(adcResult->i_in_mon_sum / adcResult->index);

    float32_t temperature = adcResult->ntc1_sum;
    temperature /= adcResult->index;
    temperature /= NTC1_R;
    temperature = logf(temperature);
    temperature = NTC1_B / temperature;
    temperature -= PMG_KELVIN_TO_CELSIUS_OFFSET;
    stat->ntc1_temp = (int16_t)temperature;

    stat->bjt_code = (uint8_t)(adcResult->bjt_code_sum / adcResult->index);
    stat->vddd = (uint16_t)(adcResult->vddd_sum / adcResult->index);

    (void)memset(adcResult, 0, sizeof(cy_stc_measure_t));

    bb_monitor();

    faults_handler();

    #if defined(DEBUG_PRINT_ENABLE) && (DEBUG_PRINT_ENABLE)
    #if defined(DEBUG_PRINT_LOG_ENABLE) && (DEBUG_PRINT_LOG_ENABLE)
    {
        char_t temp[PMG_DEBUG_PRINT_BUFFER_SIZE];
        uint32_t fault_word;
        (void)memcpy(&fault_word, &stat->faults, sizeof(fault_word));

        #define PRINT_VAR(format, var) \
                (void)sprintf(temp, format, var); \
                Cy_SCB_UART_PutString(CYBSP_UART_HW, temp); \

        PRINT_VAR("FSM %d ", stat->pmg_fsm_state);
        PRINT_VAR("EVT 0x%lx ", stat->pmg_evt);
        PRINT_VAR("FLT 0x%lx ", fault_word);
        PRINT_VAR("NTC %dC ", stat->ntc1_temp);
        PRINT_VAR("BJT %d ", stat->bjt_code);
        PRINT_VAR("VOUT %d ", stat->bb_v_out_mon);
        PRINT_VAR("IBUS %d ", stat->bb_i_in_mon * CY_PD_CUR_PER_UNIT);
        PRINT_VAR("PBUS %lu ", (long unsigned int)stat->bb_power_in);
        PRINT_VAR("NTCST %d ", stat->ntc1_state);
        PRINT_VAR("BJTST %d ", stat->bjt_state);
        PRINT_VAR("EF1 %x ", stat->ef1);
        PRINT_VAR("EF2 %x ", stat->ef2);
        PRINT_VAR("VDDD %d ", stat->vddd);
        Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n");
    }
    #else /* defined(DEBUG_PRINT_LOG_ENABLE) && (DEBUG_PRINT_LOG_ENABLE) */
    {
        char_t temp[PMG_DEBUG_PRINT_BUFFER_SIZE];
        uint32_t fault_word;
        (void)memcpy(&fault_word, &stat->faults, sizeof(fault_word));
        (void)sprintf(temp, "\r\n FSM: %d EVT: 0x%lx FAULT: 0x%lx\n",
                      stat->pmg_fsm_state, stat->pmg_evt, fault_word);
        Cy_SCB_UART_PutString(CYBSP_UART_HW, temp);
    }
    #endif /* defined(DEBUG_PRINT_LOG_ENABLE) && (DEBUG_PRINT_LOG_ENABLE) */
    #endif /* defined(DEBUG_PRINT_ENABLE) && (DEBUG_PRINT_ENABLE) */

    /* Gets the next event to be processed. */
    evt = Cy_PdUtils_EventGroup_GetEvent(&(stat->pmg_evt), true);

    #if defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE)
    /* LED mode according to FSM state and EVT */
    pmg_led_mngr();
    #endif /* defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE) */

    /* Calls the FSM handler function. */
    bb_fsm_table[stat->pmg_fsm_state](evt);
}


void Soln_BB_Task(cy_stc_pdstack_context_t* ptrPdStackContext)
{
    cy_stc_measure_t* adcResult = &(bb_context.measure);

    if (Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext,
                                     BB_MONITOR_TASK_TIMER_ID))
    {
        if (Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext,
                                         ADC_PERIOD_TIMER_ID) == false)
        {
            (void)Cy_PdUtils_SwTimer_StartWocb(ptrPdStackContext->ptrTimerContext,
                                               ADC_PERIOD_TIMER_ID,
                                               ADC_PERIOD_TIMER_PERIOD);
            adc_measurements();
            adcResult->index++;
        }
    }
    else
    {
        (void)Cy_PdUtils_SwTimer_StartWocb(ptrPdStackContext->ptrTimerContext,
                                           BB_MONITOR_TASK_TIMER_ID,
                                           BB_MONITOR_TASK_TIMER_PERIOD);

        if (adcResult->index != 0u)
        {
            output_process();
        }
    }
}


static void bb_disable(void)
{
    /* Input FET status has to be saved before CTRL_BB_IN_DIS, where it is cleared */
    bool fet_in_en = bb_context.status.bb_in_fet_status;

    bb_control_cmd(CTRL_BB_IN_DIS);

    if (fet_in_en)
    {
        /* Disable Buck-Boost */
        bb_control_cmd(CTRL_BB_DIS);

        /* Discharge C_PWR using BB. Give 5ms PFET turn off time, PWMI on, discharge 10ms, PWMI off */
        bb_control_cmd(CTRL_BB_DSCHRG);
    }
    Soln_BB_Next_Fsm_Entry(PMG_FSM_OFF);

    #if defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE)
    /* LED mode according to FSM state and EVT */
    pmg_led_mngr();
    #endif /* defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE) */
}


/* Update the Buck boost output voltage and current based on the USB-C Input power and VOUT */
static void calc_bb_out_pwr(void)
{
    cy_stc_status_t* stat = &(bb_context.status);
    stat->bb_power_in = bb_context.ptrPdStack->dpmStat.contract.minVolt;
    stat->bb_power_in *= bb_context.ptrPdStack->dpmStat.contract.curPwr;
    stat->bb_power_in /= PERCENT_DIVISOR;

    if (stat->bb_power_in >= TYPEC_MIN_PWR)
    {
        DEBUG_PRINT("\r\n Input Power sufficient");
        stat->is_vbus_pwr_sufficient = true;
    }
    else
    {
        DEBUG_PRINT("\r\n Input Power NOT sufficient");
        stat->is_vbus_pwr_sufficient = false;
    }

    DEBUG_PRINT("\r\n BB UPDATE");
    DEBUG_PRINT_VAR("\r\n Contract volt:: %d mV\n", bb_context.ptrPdStack->dpmStat.contract.minVolt);
    DEBUG_PRINT_VAR("\r\n Contract Curr:: %d mA\n",
                    bb_context.ptrPdStack->dpmStat.contract.curPwr * CY_PD_CUR_PER_UNIT);
    DEBUG_PRINT_VAR("\r\n BB Input Pwr:: %lu mW\n", (long unsigned int)stat->bb_power_in);
}


/* Buck-BoostSolution PD event handler */
void Soln_BB_PdEventHandler(cy_stc_pdstack_context_t* ptrPdStackContext,
                            cy_en_pdstack_app_evt_t evt, const void* data)
{
    const cy_stc_pdstack_pd_contract_info_t* contract_status;
    cy_stc_status_t* stat = &(bb_context.status);

    #if 0
    DEBUG_PRINT_VAR(" USBPD event is: %#x \r\n", (evt));
    #endif /* 0 */

    switch (evt)
    {
        case APP_EVT_TYPEC_STARTED:
            DEBUG_PRINT("\r\n TypeC Start");
            break;

        case APP_EVT_CONNECT:
            (void)memset(&stat->faults, 0, sizeof(stat->faults));
            stat->ntc1_state = STATE_TEMP_NORMAL;
            stat->bjt_state = STATE_TEMP_NORMAL;
            bb_control_cmd(CTRL_BB_RST);
            DEBUG_PRINT("\r\n Connect ");
            break;

        case APP_EVT_PE_DISABLED:
        /* Intentional fall-through. */
        case APP_EVT_HARD_RESET_SENT:                       /* Intentional fall through */
        case APP_EVT_HARD_RESET_RCVD:                       /* Intentional fall through */
        case APP_EVT_VBUS_PORT_DISABLE:                     /* Intentional fall through */
        case APP_EVT_DISCONNECT:                            /* Intentional fall through */
        case APP_EVT_TYPE_C_ERROR_RECOVERY:                 /* Intentional fall through */
        case APP_EVT_VBUS_UVP_FAULT:
        case APP_EVT_VBUS_OVP_FAULT:
        case APP_EVT_PR_SWAP_ACCEPTED:
        case APP_EVT_STANDBY_CURRENT:

            if (((evt == APP_EVT_HARD_RESET_SENT) || (evt == APP_EVT_HARD_RESET_RCVD)) &&
                (ptrPdStackContext->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK) &&
                (ptrPdStackContext->dpmStat.deadBat))
            {
                /* In dead-battery mode, disable the sink port to disable VBUS OVP.
                 * After a hard reset, VBUS drops below 5 V while the ADC uses VDDD as its
                 * reference. The resulting ADC measurements may be inaccurate and trigger
                 * a false OVP event. */
                ptrPdStackContext->ptrAppCbk->psnk_disable(ptrPdStackContext, NULL);
            }

            DEBUG_PRINT_VAR("\r\n USBPD event is: %#x \n", (evt));
            (void)memset(&stat->faults, 0, sizeof(stat->faults));
            stat->ntc1_state = STATE_TEMP_NORMAL;
            stat->bjt_state = STATE_TEMP_NORMAL;
            bb_disable();
            break;

        case APP_EVT_PD_CONTRACT_NEGOTIATION_COMPLETE:
            /*
             * coverity[misra_c_2012_rule_11_5_violation : SUPPRESS] The PD event callback
             * delivers the contract info payload as a const void pointer; the documented
             * type for this event is cy_stc_pdstack_pd_contract_info_t.
             */
            contract_status = (const cy_stc_pdstack_pd_contract_info_t*)data;

            if ((contract_status->status == CY_PDSTACK_CONTRACT_NEGOTIATION_SUCCESSFUL) ||
                (contract_status->status == CY_PDSTACK_CONTRACT_CAP_MISMATCH_DETECTED))
            {
                /* Check if PD power is sufficient */
                calc_bb_out_pwr();

                /* Avoid delivery power for contract mismatch */
                if (stat->is_vbus_pwr_sufficient == true)
                {
                    if (stat->pmg_fsm_state == PMG_FSM_OFF)
                    {
                        Soln_BB_Next_Fsm_Entry(PMG_FSM_INIT);
                    }
                }
                else
                {
                    Soln_BB_Next_Fsm_Entry(PMG_FSM_SHUTDOWN);
                }
            }
            break;

        default:
            break;
    }
}


static const cy_stc_sysint_t ef_intr_cfg =
{
    .intrSrc      = ioss_interrupts_gpio_2_IRQn,
    .intrPriority = 0U,
};


static void ef_handler(void)
{
    cy_stc_status_t* stat = &(bb_context.status);
    uint8_t ef1 = 0u;
    uint8_t ef2 = 0u;

    /* clear gpio interrupt */
    NVIC_ClearPendingIRQ(ef_intr_cfg.intrSrc);
    Cy_GPIO_ClearInterrupt(EF1_IN_PORT, EF1_IN_PIN);
    Cy_GPIO_ClearInterrupt(EF2_IN_PORT, EF2_IN_PIN);

    ef1 = (uint8_t)Cy_GPIO_Read(EF1_IN_PORT, EF1_IN_PIN);
    ef2 = (uint8_t)Cy_GPIO_Read(EF2_IN_PORT, EF2_IN_PIN);

    if (ef1 == 0u)
    {
        if (ef2 != 0u)
        {
            stat->faults.bb_hw_ocp = 1u;
            NVIC_DisableIRQ(ef_intr_cfg.intrSrc);
            bb_disable();
        }
        else
        {
            /* Just set a flag. Do not need to disable BB. BB takes care about OTP. */
            stat->faults.bb_hw_otp = 1u;
        }
    }
    else
    {
        if (ef2 == 0u)
        {
            stat->faults.bb_hw_ovp = 1u;
            NVIC_DisableIRQ(ef_intr_cfg.intrSrc);
            bb_disable();
        }
        else
        {
            /* Do nothing. Interrupts already cleared. */
        }
    }
    DEBUG_PRINT("\r\n EF interrupt \r\n");
}


static void rect_timer_cb(cy_timer_id_t id, /**< Timer ID for which callback is being generated. */
                          void* callbackContext)       /**< Timer module Context. */
{
    CY_UNUSED_PARAMETER(id);               /* Unused parameter */
    CY_UNUSED_PARAMETER(callbackContext);  /* Unused parameter */

    Cy_GPIO_Write(RECT_PORT, RECT_PIN, 1u);
}


static void pwmi_timer_cb(cy_timer_id_t id, /**< Timer ID for which callback is being generated. */
                          void* callbackContext)       /**< Timer module Context. */
{
    CY_UNUSED_PARAMETER(id);               /* Unused parameter */
    CY_UNUSED_PARAMETER(callbackContext);  /* Unused parameter */

    Cy_GPIO_Write(PWMI_OUT_PORT, PWMI_OUT_PIN, 1u);
    DEBUG_PRINT("\r\n BB PWMI Switch ON ..\r\n");
}


static void fet_in_timer_cb(cy_timer_id_t id, /**< Timer ID for which callback is being generated. */
                            void* callbackContext)       /**< Timer module Context. */
{
    CY_UNUSED_PARAMETER(id);               /* Unused parameter */
    CY_UNUSED_PARAMETER(callbackContext);  /* Unused parameter */

    bb_context.status.pmg_evt |= PMG_EVT_IN_SS_CMLT;
}


static void bb_discharge_timer_cb(cy_timer_id_t id, /**< Timer ID for which callback is being generated. */
                                  void* callbackContext)       /**< Timer module Context. */
{
    CY_UNUSED_PARAMETER(id);               /* Unused parameter */
    CY_UNUSED_PARAMETER(callbackContext);  /* Unused parameter */
    Cy_GPIO_Write(PWMI_OUT_PORT, PWMI_OUT_PIN, 0u);
    bb_control_cmd(CTRL_BB_RST);
}


static void pfet_turn_off_timer_cb(cy_timer_id_t id,       /**< Timer ID for which callback is being generated. */
                                   void* callbackContext)  /**< Timer module Context. */
{
    CY_UNUSED_PARAMETER(id);               /* Unused parameter */
    CY_UNUSED_PARAMETER(callbackContext);  /* Unused parameter */
    Cy_GPIO_Write(PWMI_OUT_PORT, PWMI_OUT_PIN, 1u);
    (void)Cy_PdUtils_SwTimer_Start(bb_context.ptrPdStack->ptrTimerContext,
                                   (void*)bb_context.ptrPdStack,
                                   (cy_timer_id_t)BB_DSCHRG_TIMER_ID,
                                   BB_DSCHRG_TIMER_PERIOD,
                                   &bb_discharge_timer_cb);
}


static void bb_control_cmd(const bb_control_cmd_t src_cmd)
{
    switch (src_cmd)
    {
        case CTRL_BB_DIS:
            Cy_PdUtils_SwTimer_Stop(bb_context.ptrPdStack->ptrTimerContext,
                                    (cy_timer_id_t)RECT_TIMER_ID);
            Cy_PdUtils_SwTimer_Stop(bb_context.ptrPdStack->ptrTimerContext,
                                    (cy_timer_id_t)PWMI_TIMER_ID);
            Cy_PdUtils_SwTimer_Stop(bb_context.ptrPdStack->ptrTimerContext,
                                    (cy_timer_id_t)VOUT_UNEXP_TIMER_ID);
            NVIC_DisableIRQ(ef_intr_cfg.intrSrc);
            Cy_GPIO_Write(PWMI_OUT_PORT, PWMI_OUT_PIN, 0u);
            DEBUG_PRINT("\r\n BB PWMI Switch OFF ..\r\n");
            break;

        case CTRL_BB_RST:
        {
            /* BB reset causes VDDD (reference for ADC) drop. To avoid VBUS_OVP disallow and clear interrupt. */
            uint32_t state = Cy_SysLib_EnterCriticalSection();

            Cy_GPIO_Write(BUCK_BOOST_EN_PORT, BUCK_BOOST_EN_PIN, 1u);
            Cy_SysLib_DelayUs(BB_RESET_ASSERT_DELAY_US); /* Wait BB detect reset, to reactivate soft-start routine */
            Cy_GPIO_Write(BUCK_BOOST_EN_PORT, BUCK_BOOST_EN_PIN, 0u);
            Cy_SysLib_DelayUs(BB_RESET_RECOVERY_DELAY_US);

            bb_context.ptrPdStack->ptrUsbPdContext->base->intr1 = PDSS_INTR1_CMP_OUT1_CHANGED |
                                                                  PDSS_INTR1_CMP_OUT2_CHANGED;
            Cy_SysLib_ExitCriticalSection(state);
            break;
        }

        case CTRL_BB_EN:
            Cy_GPIO_Write(RECT_PORT, RECT_PIN, 0u);
            (void)Cy_PdUtils_SwTimer_Start(bb_context.ptrPdStack->ptrTimerContext,
                                           (void*)bb_context.ptrPdStack,
                                           (cy_timer_id_t)RECT_TIMER_ID,
                                           RECT_TIMER_PERIOD,
                                           &rect_timer_cb);
            (void)Cy_PdUtils_SwTimer_Start(bb_context.ptrPdStack->ptrTimerContext,
                                           (void*)bb_context.ptrPdStack,
                                           (cy_timer_id_t)PWMI_TIMER_ID,
                                           PWMI_TIMER_PERIOD,
                                           &pwmi_timer_cb);

            /* configure and enable pin interrupt */
            (void)Cy_SysInt_Init(&ef_intr_cfg, &ef_handler);
            Cy_GPIO_SetInterruptEdge(EF1_IN_PORT, EF1_IN_PIN, CY_GPIO_INTR_FALLING);
            Cy_GPIO_SetInterruptEdge(EF2_IN_PORT, EF2_IN_PIN, CY_GPIO_INTR_FALLING);
            NVIC_ClearPendingIRQ(ef_intr_cfg.intrSrc);
            Cy_GPIO_ClearInterrupt(EF1_IN_PORT, EF1_IN_PIN);
            Cy_GPIO_ClearInterrupt(EF2_IN_PORT, EF2_IN_PIN);
            NVIC_EnableIRQ(ef_intr_cfg.intrSrc);
            break;

        case CTRL_BB_IN_EN:
            Cy_GPIO_Write(VBUS_C_CTRL_PORT, VBUS_C_CTRL_PIN, 1u);
            bb_context.status.bb_in_fet_status = true;
            (void)Cy_PdUtils_SwTimer_Start(bb_context.ptrPdStack->ptrTimerContext,
                                           (void*)bb_context.ptrPdStack,
                                           (cy_timer_id_t)SOFTSTART_FET_IN_TIMER_ID,
                                           SOFTSTART_FET_IN_TIMER_PERIOD,
                                           &fet_in_timer_cb);
            DEBUG_PRINT("\r\n VBUS FET ON\r\n");
            break;

        case CTRL_BB_IN_DIS:
            Cy_GPIO_Write(VBUS_C_CTRL_PORT, VBUS_C_CTRL_PIN, 0u);
            bb_context.status.bb_in_fet_status = false;
            Cy_PdUtils_SwTimer_Stop(bb_context.ptrPdStack->ptrTimerContext,
                                    (cy_timer_id_t)SOFTSTART_FET_IN_TIMER_ID);
            DEBUG_PRINT("\r\n VBUS Consumer FET OFF\r\n");
            break;

        case CTRL_BB_DSCHRG:
            (void)Cy_PdUtils_SwTimer_Start(bb_context.ptrPdStack->ptrTimerContext,
                                           (void*)bb_context.ptrPdStack,
                                           (cy_timer_id_t)PFET_TURN_OFF_TIMER_ID,
                                           PFET_TURN_OFF_TIMER_PERIOD,
                                           &pfet_turn_off_timer_cb);
            break;

        default:
            break;
    }
}


#if defined(VBUS_OVP_ENABLE) && (VBUS_OVP_ENABLE)
void soln_fault_vbus_ovp_enable(cy_stc_pdstack_context_t* context, uint16_t volt_mV, bool pfet,
                                cy_cb_vbus_fault_t ovp_cb)
{
    (void)pfet; /* Unused parameter */

    /* Calculate required VBUS for OVP. */
    uint8_t level = Cy_USBPD_Adc_GetVbusLevel(context->ptrUsbPdContext, 
                                              CY_USBPD_ADC_ID_0,
                                              volt_mV,
                                              GET_VBUS_OVP_TABLE(
                                                  context->ptrUsbPdContext)->threshold);

    /* Use the ADC comparator to monitor VBus OVP voltage */
    Cy_USBPD_Adc_CompCtrl(context->ptrUsbPdContext,
                          CY_USBPD_ADC_ID_0,
                          CY_USBPD_ADC_INPUT_AMUX_A,
                          level,
                          CY_USBPD_ADC_INT_FALLING,
                          (cy_cb_adc_events_t)ovp_cb);
}


void soln_fault_vbus_ovp_disable(cy_stc_pdstack_context_t* context)
{
    /* Disable the ADC comparator for VBus OVP */
    Cy_USBPD_Adc_CompCtrl(context->ptrUsbPdContext,
                          CY_USBPD_ADC_ID_0,
                          CY_USBPD_ADC_INPUT_AMUX_A,
                          0u,
                          CY_USBPD_ADC_INT_DISABLED,
                          NULL);
}


#endif /* defined(VBUS_OVP_ENABLE) && (VBUS_OVP_ENABLE) */


#if defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE)
static void pmg_led_mngr(void)
{
    cy_stc_status_t* stat = &(bb_context.status);

    if ((stat->faults.bb_sw_ocp != 0u) ||
        (stat->faults.bb_sw_ovp != 0u) ||
        (stat->faults.bb_sw_uvp != 0u) ||
        (stat->faults.bb_hw_ocp != 0u) ||
        (stat->faults.bb_hw_ovp != 0u))
    {
        /* Latch the fault indication. The individual fault flags can be
         * re-evaluated and cleared by the monitor on subsequent cycles (e.g.
         * read_bb_error_flag() clears the BB flags once the error-flag pins
         * recover after BB shutdown). The latch keeps the LED solid ON until
         * the fault state is reset at the next APP_EVT_CONNECT/disconnect. */
        stat->faults.fault_active = 1u;
    }

    if ((stat->faults.fault_active != 0u) || (stat->faults.bb_vout_init != 0u))
    {
        Cy_App_LedCtrl_SwitchMode(CY_APP_LED_CTRL_LED_ON, &led_context);
    }
    else
    {
        switch (stat->pmg_fsm_state)
        {
            case PMG_FSM_OFF:
            case PMG_FSM_SHUTDOWN:
                Cy_App_LedCtrl_SwitchMode(CY_APP_LED_CTRL_LED_OFF, &led_context);
                break;

            case PMG_FSM_DELIVERY:
                Cy_App_LedCtrl_SwitchMode(CY_APP_LED_CTRL_LED_BLINKING, &led_context);

                /* LED API uses 1000ms as a default blink_rate. Need to override. */
                led_context.blink_rate = LED_PD_SNK_TIMER_PERIOD;
                break;

            case PMG_FSM_COOLDOWN:
                Cy_App_LedCtrl_SwitchMode(CY_APP_LED_CTRL_LED_BREATHING, &led_context);
                break;

            default:
                break;
        }
    }
}
#endif /* defined(CY_APP_LED_CONTROL_ENABLE) && (CY_APP_LED_CONTROL_ENABLE) */


/* [] END OF FILE */
