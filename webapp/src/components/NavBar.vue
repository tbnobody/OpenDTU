<template>
  <nav class="navbar navbar-expand-md navbar-dark fixed-top bg-dark">
    <div class="container-fluid">
      <a class="navbar-brand" href="#">OpenDTU</a>
      <button
        class="navbar-toggler"
        type="button"
        data-bs-toggle="collapse"
        data-bs-target="#navbarNavAltMarkup"
        aria-controls="navbarNavAltMarkup"
        aria-expanded="false"
        aria-label="Toggle navigation"
      >
        <span class="navbar-toggler-icon"></span>
      </button>
      <div class="collapse navbar-collapse" id="navbarNavAltMarkup">
        <div class="navbar-nav">
          <li class="nav-item">
            <router-link class="nav-link" to="/">Home</router-link>
          </li>
          <li class="nav-item dropdown">
            <a
              class="nav-link dropdown-toggle"
              href="#"
              id="navbarScrollingDropdown"
              role="button"
              data-bs-toggle="dropdown"
              aria-expanded="false"
            >
              Settings
            </a>
            <ul class="dropdown-menu" aria-labelledby="navbarScrollingDropdown">
              <li>
                <router-link class="dropdown-item" to="/settings/network"
                  >Network Settings</router-link
                >
              </li>
              <li>
                <router-link class="dropdown-item" to="/settings/ntp"
                  >NTP Settings</router-link
                >
              </li>
              <li>
                <router-link class="dropdown-item" to="/settings/mqtt"
                  >MqTT Settings</router-link
                >
              </li>
            </ul>
          </li>
          <li class="nav-item dropdown">
            <a
              class="nav-link dropdown-toggle"
              href="#"
              id="navbarScrollingDropdown"
              role="button"
              data-bs-toggle="dropdown"
              aria-expanded="false"
            >
              Info
            </a>
            <ul class="dropdown-menu" aria-labelledby="navbarScrollingDropdown">
              <li>
                <router-link class="dropdown-item" to="/info/system"
                  >System</router-link
                >
              </li>
              <li>
                <router-link class="dropdown-item" to="/info/network"
                  >Network</router-link
                >
              </li>
              <li>
                <router-link class="dropdown-item" to="/info/ntp"
                  >NTP</router-link
                >
              </li>
            </ul>
          </li>
          <li class="nav-item">
            <router-link class="nav-link" to="/about">About</router-link>
          </li>
        </div>
      </div>
    </div>
  </nav>
</template>

<script>
</script>

<style>
</style>