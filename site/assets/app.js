'use strict';
const search = document.getElementById('search');
const filters = [...document.querySelectorAll('[data-filter]')];
const cards = [...document.querySelectorAll('[data-category]')];
let category = 'all';
function filterProjects() {
  const query = search.value.trim().toLocaleLowerCase('en');
  let count = 0;
  for (const card of cards) {
    const matches = (category === 'all' || card.dataset.category === category) && card.dataset.search.includes(query);
    card.hidden = !matches;
    if (matches) count++;
  }
  document.getElementById('empty-state').hidden = count > 0;
  document.getElementById('results').textContent = `${count} project${count === 1 ? '' : 's'} shown`;
  for (const button of filters) {
    const active = button.dataset.filter === category;
    button.classList.toggle('active', active);
    button.setAttribute('aria-pressed', String(active));
  }
}
for (const button of filters) button.addEventListener('click', () => {category = button.dataset.filter; filterProjects();});
search.addEventListener('input', filterProjects);
document.getElementById('reset').addEventListener('click', () => {category = 'all';search.value = '';filterProjects();search.focus();});
// A deep link to a project should reveal it even after filtering the collection.
function revealAnchor() {
  if (['#emergency4', '#mod-launcher'].includes(location.hash)) {
    category = 'all'; search.value = ''; filterProjects();
  }
}
window.addEventListener('hashchange', revealAnchor);
revealAnchor();

const stage = document.getElementById('comparison');
const slider = document.getElementById('reveal');
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
  document.getElementById('image-status').textContent = 'A screenshot could not load. Refresh the page or open the full comparison.';
});
