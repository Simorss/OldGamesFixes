'use strict';
const search = document.getElementById('search');
const cards = [...document.querySelectorAll('[data-category]')];
const games = [...document.querySelectorAll('[data-game]')];
function filterProjects() {
  const query = search.value.trim().toLocaleLowerCase('en');
  for (const card of cards) card.hidden = !card.dataset.search.includes(query);
  let count = 0;
  for (const game of games) {
    game.hidden = [...game.querySelectorAll('[data-category]')].every(card => card.hidden);
    if (!game.hidden) count++;
  }
  document.getElementById('empty-state').hidden = count > 0;
  document.getElementById('results').textContent = `${count} game${count === 1 ? '' : 's'} shown`;
}
search.addEventListener('input', filterProjects);
document.getElementById('reset').addEventListener('click', () => {search.value = '';filterProjects();search.focus();});
const bonusMenus = [...document.querySelectorAll('.bonus-downloads')];
document.addEventListener('click', event => {
  for (const menu of bonusMenus) if (!menu.contains(event.target)) menu.open = false;
});
document.addEventListener('keydown', event => {
  if (event.key !== 'Escape') return;
  for (const menu of bonusMenus) {
    if (!menu.open) continue;
    const focused = menu.contains(document.activeElement);
    menu.open = false;
    if (focused) menu.querySelector('summary').focus();
  }
});
// A deep link to a project should reveal it even after filtering the collection.
function revealAnchor() {
  if (document.getElementById(location.hash.slice(1))?.closest('[data-game]')) {
    search.value = ''; filterProjects();
  }
}
window.addEventListener('hashchange', revealAnchor);
revealAnchor();

// Each game owns its own comparison controls and image status.
for (const visual of document.querySelectorAll('.project-visual')) {
  const stage = visual.querySelector('.comparison');
  const slider = visual.querySelector('input[type="range"]');
  if (!stage || !slider) continue;
  function setPosition(value) {
    const position = Math.max(0, Math.min(100, Number(value)));
    stage.style.setProperty('--reveal', `${position}%`);
    slider.value = String(position);
    slider.setAttribute('aria-valuetext', `${Math.round(position)}% without fix, ${Math.round(100 - position)}% with fix`);
  }
  slider.addEventListener('input', () => setPosition(slider.value));
  function fromPointer(event) {
    const bounds = stage.getBoundingClientRect();
    setPosition((event.clientX - bounds.left) / bounds.width * 100);
  }
  stage.addEventListener('pointerdown', event => {
    if (event.button !== 0 || !event.isPrimary) return;
    stage.setPointerCapture(event.pointerId);
    fromPointer(event);
  });
  stage.addEventListener('pointermove', event => {if (stage.hasPointerCapture(event.pointerId)) fromPointer(event);});
  function releasePointer(event) {if (stage.hasPointerCapture(event.pointerId)) stage.releasePointerCapture(event.pointerId);}
  stage.addEventListener('pointerup', releasePointer);
  stage.addEventListener('pointercancel', releasePointer);
  Promise.all([...stage.querySelectorAll('img')].map(image => image.decode())).catch(() => {
    const status = visual.querySelector('[role="status"]');
    if (status) status.textContent = 'A screenshot could not load. Refresh the page or open the full comparison.';
  });
  setPosition(slider.value);
}
