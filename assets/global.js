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