/***************************************************************************//**
* \file main.c
* \version 1.0
*
* \brief This is the source code for the PMG1B2 MCU: USBPD Sink Example
*        for ModusToolbox.
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

#include "cybsp.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_usbpd_common.h"
#include "cy_usbpd_typec.h"
#include "cy_pdstack_dpm.h"
#include "cy_app_instrumentation.h"
#include "cy_app_debug.h"
#include "cy_app.h"
#include "cy_app_pdo.h"
#include "cy_app_fault_handlers.h"
#include "cy_app_sink.h"
#include "cy_app_swap.h"
#include "cy_app_vdm.h"
#include "cy_solution.h"
#include "mtbcfg_ezpd.h"

cy_stc_pdutils_sw_timer_t gl_TimerCtx;
cy_stc_usbpd_context_t gl_UsbPdPort0Ctx;
cy_stc_pdstack_context_t gl_PdStackPort0Ctx;

#if DEBUG_PRINT_ENABLE
cy_stc_scb_uart_context_t CYBSP_UART_context;
#endif /* DEBUG_PRINT_ENABLE */

const cy_stc_pdstack_dpm_params_t pdstack_port0_dpm_params =
{
    .dpmSnkWaitCapPeriod = 400,
    .dpmRpAudioAcc = CY_PD_RP_TERM_RP_CUR_DEF,
    .dpmDefCableCap = 300,
    .muxEnableDelayPeriod = 0,
    .typeCSnkWaitCapPeriod = 0,
    .defCur = 90
};

const cy_stc_sysint_t wdt_interrupt_config =
{
    .intrSrc = (IRQn_Type)srss_interrupt_wdt_IRQn,
    .intrPriority = 1U,
};

const cy_stc_sysint_t usbpd_port0_intr0_config =
{
    .intrSrc = (IRQn_Type)mtb_usbpd_port0_IRQ,
    .intrPriority = 1U,
};

const cy_stc_sysint_t usbpd_port0_intr1_config =
{
    .intrSrc = (IRQn_Type)mtb_usbpd_port0_DS_IRQ,
    .intrPriority = 1U,
};

/* Solution PD event handler */
void sln_pd_event_handler(cy_stc_pdstack_context_t *ctx, cy_en_pdstack_app_evt_t evt, const void *data)
{
    Soln_BB_PdEventHandler(ctx, evt, data);
}

void instrumentation_cb(uint8_t port, cy_en_inst_evt_t evt)
{
#if (CY_APP_DEBUG_ENABLE)
    if(evt == CY_APP_INST_EVT_WDT_RESET)
    {
        CY_APP_DEBUG_LOG(0, CY_APP_DEBUG_WDT, NULL, 0, CY_APP_DEBUG_LOGLEVEL_CRITICAL, true);
    }
    else if(evt == CY_APP_INST_EVT_HARD_FAULT)
    {
        CY_APP_DEBUG_LOG(0, CY_APP_DEBUG_HARD_FAULT, NULL, 0, CY_APP_DEBUG_LOGLEVEL_CRITICAL, true);
    }
    else if(evt == CY_APP_INST_EVT_POWER_CYCLE)
    {
        CY_APP_DEBUG_LOG(0, CY_APP_DEBUG_POWER_CYCLE, NULL, 0, CY_APP_DEBUG_LOGLEVEL_CRITICAL, true);
    }
    else
    {
        /* Do Nothing */
    }
#else
    CY_UNUSED_PARAMETER(port);
    CY_UNUSED_PARAMETER(evt);
#endif /* (CY_APP_DEBUG_ENABLE) */
}

static void wdt_interrupt_handler(void)
{
    /* Clear WDT pending interrupt */
    Cy_WDT_ClearInterrupt();

    /* Invoke the timer handler. */
    Cy_PdUtils_SwTimer_InterruptHandler(&(gl_TimerCtx));
}

static void cy_usbpd0_intr0_handler(void)
{
    Cy_USBPD_Intr0Handler(&gl_UsbPdPort0Ctx);
}

static void cy_usbpd0_intr1_handler(void)
{
    Cy_USBPD_Intr1Handler(&gl_UsbPdPort0Ctx);
}

cy_stc_pd_dpm_config_t *get_dpm_connect_stat(void)
{
    return &(gl_PdStackPort0Ctx.dpmConfig);
}

/*
 * Application callback functions for the DPM. Since this application
 * uses the functions provided by the stack, loading the stack defaults.
 */
const cy_stc_pdstack_app_cbk_t app_callback =
{
    .app_event_handler = Cy_App_EventHandler,
    .vconn_enable = Cy_App_VconnEnable,
    .vconn_disable = Cy_App_VconnDisable,
    .vconn_is_present = Cy_App_VconnIsPresent,
    .vbus_is_present = Cy_App_VbusIsPresent,
    .vbus_discharge_on = Cy_App_VbusDischargeOn,
    .vbus_discharge_off = Cy_App_VbusDischargeOff,
    .psnk_set_voltage = Cy_App_Sink_SetVoltage,
    .psnk_set_current = Cy_App_Sink_SetCurrent,
    .psnk_enable = Cy_App_Sink_Enable,
    .psnk_disable = Cy_App_Sink_Disable,
    .eval_src_cap = Cy_App_Pdo_EvalSrcCap,
    .eval_dr_swap = Cy_App_Swap_EvalDrSwap,
    .eval_pr_swap = Cy_App_Swap_EvalPrSwap,
    .eval_vconn_swap = Cy_App_Swap_EvalVconnSwap,
    .eval_vdm = Cy_App_Vdm_EvalVdmMsg,
    .vbus_get_value = Cy_App_VbusGetValue,
};

uint32_t gl_discIdResp[4] = {0xFF00A841UL, 0x184004B4UL, 0x00000000UL, 0xF5110000UL};

const cy_stc_app_params_t app_params =
{
    .appVbusPollAdcId = CY_USBPD_ADC_ID_0,
    .appVbusPollAdcInput = CY_USBPD_ADC_INPUT_AMUX_A,
    .prefPowerRole = 2u, /* 0-Sink, 1-Source, 2-No Preference */
    .prefDataRole = 2u, /* 0-UFP, 1- DFP, 2-No Preference */
    .discIdResp = (cy_pd_pd_do_t *)&gl_discIdResp[0],
    .discIdLen = 0x14,
    .swapResponse = 0x3F
};

int main(void)
{
    cy_rslt_t result;
    cy_stc_pdutils_timer_config_t timerConfig;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /*
     * Register the interrupt handler for the watchdog timer. This timer is used to
     * implement the soft timers required by the USB-PD Stack.
     */
    Cy_SysInt_Init(&wdt_interrupt_config, &wdt_interrupt_handler);
    NVIC_EnableIRQ(wdt_interrupt_config.intrSrc);
    timerConfig.sys_clk_freq = Cy_SysClk_ClkSysGetFrequency();
    timerConfig.hw_timer_ctx = NULL;

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize the instrumentation related data structures. */
    Cy_App_Instrumentation_Init(&gl_TimerCtx);

    /* Register callback function to be executed when instrumentation fault occurs. */
    Cy_App_Instrumentation_RegisterCb((cy_app_instrumentation_cb_t)instrumentation_cb);

    /* Configure and enable the USBPD interrupts */
    Cy_SysInt_Init(&usbpd_port0_intr0_config, &cy_usbpd0_intr0_handler);
    NVIC_EnableIRQ(usbpd_port0_intr0_config.intrSrc);

    Cy_SysInt_Init(&usbpd_port0_intr1_config, &cy_usbpd0_intr1_handler);
    NVIC_EnableIRQ(usbpd_port0_intr1_config.intrSrc);

    /* Initialize the USBPD driver */
    Cy_USBPD_Init(&gl_UsbPdPort0Ctx, 0, mtb_usbpd_port0_HW, mtb_usbpd_port0_HW_TRIM,
            (cy_stc_usbpd_config_t *)&mtb_usbpd_port0_config, get_dpm_connect_stat);

    /* Initialize the soft timer module. */
    Cy_PdUtils_SwTimer_Init(&gl_TimerCtx, &timerConfig);

    /* Initialize the Device Policy Manager. */
    Cy_PdStack_Dpm_Init(&gl_PdStackPort0Ctx,
                        &gl_UsbPdPort0Ctx,
                        &mtb_usbpd_port0_pdstack_config,
                        ((cy_stc_pdstack_app_cbk_t *)(&app_callback)),
                        &pdstack_port0_dpm_params,
                        &gl_TimerCtx);

    /* Initialize the fault configuration values */
    Cy_App_Fault_InitVars(&gl_PdStackPort0Ctx);

    /* Perform application level initialization. */
    Cy_App_Init(&gl_PdStackPort0Ctx, &app_params);

    Soln_BB_Init(&gl_PdStackPort0Ctx);

    sln_pd_event_handler(&gl_PdStackPort0Ctx, APP_EVT_POWER_CYCLE, NULL);

    /* Start any timers or tasks associated with application instrumentation. */
    Cy_App_Instrumentation_Start();

    /* Start the device policy manager operation. This will initialize the USB-PD block and enable connect detection. */
    Cy_PdStack_Dpm_Start(&gl_PdStackPort0Ctx);

#if DEBUG_PRINT_ENABLE
    Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, &CYBSP_UART_context);
    Cy_SCB_UART_Enable(CYBSP_UART_HW);

    /* Sequence to clear screen */
    Cy_SCB_UART_PutString(CYBSP_UART_HW,"\x1b[2J\x1b[;H");

    /* Print "PWM LED" */
    Cy_SCB_UART_PutString(CYBSP_UART_HW,"****************** ");
    Cy_SCB_UART_PutString(CYBSP_UART_HW,"PMG1 USBPD Sink");
    Cy_SCB_UART_PutString(CYBSP_UART_HW,"****************** \r\n\n");
#endif /* DEBUG_PRINT_ENABLE */

    /*
     * After the initialization is complete, keep processing the USB-PD device
     * policy manager task in a loop.  Since this application does not have any
     * other function, the PMG1 device can be placed in "deep sleep" mode for
     * power saving whenever the PD stack and drivers are idle.
     */
    for (;;)
    {
        /* Handle the device policy tasks for each PD port. */
        Cy_PdStack_Dpm_Task(&gl_PdStackPort0Ctx);
        
        /* Perform any application level tasks. */
        Cy_App_Task(&gl_PdStackPort0Ctx);

        /* Perform tasks associated with instrumentation. */
        Cy_App_Instrumentation_Task();

#if SYS_DEEPSLEEP_ENABLE
        if(gl_PdStackPort0Ctx.dpmConfig.connect == true)
#endif /* SYS_DEEPSLEEP_ENABLE */
        {
            Soln_BB_Task(&gl_PdStackPort0Ctx);
        }

#if SYS_DEEPSLEEP_ENABLE
        /* Enter deep sleep mode at disconnect for power saving. */
        if(gl_PdStackPort0Ctx.dpmConfig.connect == false)
        {
#if DEBUG_PRINT_ENABLE
            if (Cy_SCB_IsTxComplete(CYBSP_UART_HW))
#endif /* DEBUG_PRINT_ENABLE */
            {
                Cy_App_SystemSleep(&gl_PdStackPort0Ctx, NULL);
            }
        }
#endif /* SYS_DEEPSLEEP_ENABLE */
    }
}

/* [] END OF FILE */
