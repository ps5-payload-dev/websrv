/* Copyright (C) 2025 John Törnblom

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


async function main() {
    const PAYLOAD_NAME = 'pkg_install.elf'
    const PAYLOAD_PATH = window.workingDir + '/' + PAYLOAD_NAME
    const INSTALL_ALL_PREFIX = '__install_all__:'

    async function drainStream(stream) {
        if (!stream) return
        const reader = stream.getReader()
        while (true) {
            const { done } = await reader.read()
            if (done) break
        }
    }

    function joinPath(base, name) {
        if (base.startsWith('/')) {
            if (!base.endsWith('/')) base += '/'
            return base + name
        }
        const u = new URL(base)
        let p = u.pathname || '/'
        if (!p.endsWith('/')) p += '/'
        p += encodeURIComponent(name)
        u.pathname = p
        return u.toString()
    }

    async function installAllInDir(dirPath) {
        const listing = await ApiClient.fsListDir(dirPath)
        if (!listing.data) {
            alert('Failed to load directory: ' + redactUrlPassword(dirPath) + '\nError code: ' + listing.status)
            return
        }

        const pkgs = listing.data
            .filter(e => e && !e.isDir() && e.name && e.name.toLowerCase().endsWith('.pkg'))
            .map(e => joinPath(dirPath, e.name))

        if (pkgs.length === 0) {
            alert('No .pkg found in: ' + redactUrlPassword(dirPath))
            return
        }

        for (let i = 0; i < pkgs.length; i++) {
            let pkg = pkgs[i]
            if (!pkg.startsWith('/')) {
                pkg = ApiClient.getNetworkShareHttpProxyUrl(pkg)
            }
            const r = await ApiClient.launchApp(PAYLOAD_PATH, [PAYLOAD_NAME, pkg], null, null, true)
            await drainStream(r.data)
        }

        alert('Install all finished.\nCount: ' + pkgs.length)
    }

    return {
        mainText: 'PKG installer',
        secondaryText: 'Install a PKG file',
        onclick: async () => {
            let sel = await pickPath('', 'Select PKG...', true, 'pkginstall')
            if (!sel) return

            if (sel.startsWith(INSTALL_ALL_PREFIX)) {
                await installAllInDir(sel.substring(INSTALL_ALL_PREFIX.length))
                return
            }

            if (!sel.startsWith('/')) {
                sel = ApiClient.getNetworkShareHttpProxyUrl(sel)
            }

            return {
                path: PAYLOAD_PATH,
                args: [PAYLOAD_NAME, sel],
                daemon: true
            }
        }
    }
}