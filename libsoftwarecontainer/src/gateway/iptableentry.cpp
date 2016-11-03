/*
 * Copyright (C) 2016 Pelagicore AB
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * For further information see LICENSE
 */

#include "iptableentry.h"
#include "gateway.h"

ReturnCode IPTableEntry::applyRules()
{
    if (ReturnCode::FAILURE == setPolicy()) {
        log_error() << "Unable to set policy " << convertTarget(m_defaultTarget) << " for " << m_type;
        return ReturnCode::FAILURE;
    }

    for (auto rule : m_rules) {
        if (ReturnCode::FAILURE == insertRule(rule)) {
            log_error() << "Couldn't apply the rule " << rule.target;
            return ReturnCode::FAILURE;
        }
    }

    return ReturnCode::SUCCESS;
}

std::string IPTableEntry::convertTarget (Target& t)
{
    switch(t)
    {
        case Target::ACCEPT:
            return "ACCEPT";
        case Target::DROP:
            return "DROP";
        case Target::REJECT:
            return "REJECT";
        case Target::INVALID_TARGET:
            return "INVALID";
    }
    return "INVALID";
}

ReturnCode IPTableEntry::insertRule(Rule rule)
{
    std::string iptableCommand = "iptables -A " + m_type;

    if (!rule.host.empty()) {
        if ("INPUT" == m_type) {
            iptableCommand = iptableCommand + " -s ";
        } else {
            iptableCommand = iptableCommand + " -d ";
        }
        iptableCommand = iptableCommand + rule.host ;
    }

    if (rule.ports.any) {
        if (rule.ports.multiport) {
            iptableCommand = iptableCommand + " -p tcp --match multiport ";

            if ("INPUT" == m_type) {
                iptableCommand = iptableCommand + "--sports " + rule.ports.ports;
            } else {
                iptableCommand = iptableCommand + "--dports " + rule.ports.ports;
            }
        } else {
            iptableCommand = iptableCommand + " -p tcp ";
            if ("INPUT" == m_type) {
                iptableCommand = iptableCommand + "--sport " + rule.ports.ports;
            } else {
                iptableCommand = iptableCommand + "--dport " + rule.ports.ports;
            }
        }
    }

    iptableCommand = iptableCommand + " -j " + convertTarget(rule.target);
    log_debug() << "Add network rule : " <<  iptableCommand;

    try {
        Glib::spawn_command_line_sync(iptableCommand);

    } catch (Glib::SpawnError e) {
        log_error() << "Failed to spawn " << iptableCommand << ": code " << e.code()
                               << " msg: " << e.what();

        return ReturnCode::FAILURE;
    } catch (Glib::ShellError e) {
        log_error() << "Failed to call " << iptableCommand << ": code " << e.code()
                                       << " msg: " << e.what();
        return ReturnCode::FAILURE;
    }

    return ReturnCode::SUCCESS;
}


ReturnCode IPTableEntry::setPolicy()
{
    if (m_defaultTarget != Target::ACCEPT && m_defaultTarget != Target::DROP) {
        log_error() << "Wrong default target : " << convertTarget(m_defaultTarget);
    }

    std::string iptableCommand = "iptables -P " + m_type + " " + convertTarget(m_defaultTarget);

    try {
        Glib::spawn_command_line_sync(iptableCommand);

    } catch (Glib::SpawnError e) {
        log_error() << "Failed to set policy " << iptableCommand << ": code " << e.code()
                               << " msg: " << e.what();

        return ReturnCode::FAILURE;
    } catch (Glib::ShellError e) {
        log_error() << "Failed to set policy " << iptableCommand << ": code " << e.code()
                                       << " msg: " << e.what();
        return ReturnCode::FAILURE;
    }

    return ReturnCode::SUCCESS;
}