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
 * @typedef {Object} Navigo
 * @property {(path: string) => void} navigate
 * @property {((func: () => void) => Navigo) & ((path: string, func: () => void) => Navigo)} on
 * @property {() => Navigo} resolve 
 */

/**
 * @typedef {Object} Globals
 * @property {Navigo} Router
 * @property {Object} Funcs
 * @property {string?} hbApiJs
 * @property {string?} ApiClientJs
 * @property {number?} removeScrollingClassFromCarouselTimeoutId
 */

/** @type {Globals} */
const Globals = {
    // @ts-ignore
    Router: new Navigo("/", { hash: true }),
    Funcs: {},
    hbApiJs: null,
    ApiClientJs: null,
    removeScrollingClassFromCarouselTimeoutId: null
};
