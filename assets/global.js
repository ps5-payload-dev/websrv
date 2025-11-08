/* Copyright (C) 2024 idlesauce

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */


// @ts-check

/**
 * @typedef {Object} Globals
 * @property {Router} Router
 * @property {string?} hbApiJs
 * @property {string?} ApiClientJs
 * @property {number?} removeScrollingClassFromCarouselTimeoutId
 * @property {number} lastCarouselLeftRightKeyDownTimestamp
 */

/** @type {Globals} */
var Globals = {
    // @ts-ignore
    Router: null,
    hbApiJs: null,
    ApiClientJs: null,
    lastCarouselLeftRightKeyDownTimestamp: 0,
};

const DIR_ICON = "dir-icon";
const FILE_ICON = "file-icon";
const BACK_ICON = "back-icon";
const ADD_ICON = "add-icon";
const HDD_ICON = "hdd-icon";
const SMB_SHARE_ICON = "smb-share-icon";
const USB_ICON = "usb-icon";
const MORE_ICON = "more-icon";
const DELETE_ICON = "delete-icon";
const EDIT_ICON = "edit-icon";
const LAN_ICON = "lan-icon";
const FOLDER_CHECK_ICON = "folder-check-2-icon";

const PS_TRIANGLE_ICON = "ps-triangle-icon";
const PS_CIRCLE_ICON = "ps-circle-icon";
const PS_CROSS_ICON = "ps-cross-icon";
const PS_SQUARE_ICON = "ps-square-icon";

const PS_ICON = "ps-icon";